#include <obs.h>
#include <obs-module.h>
#include <graphics/graphics.h>
#include <graphics/vec4.h>

#include <algorithm>
#include <cstdint>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

namespace {
constexpr uint32_t W=256,H=256;
using Frame=std::vector<uint8_t>;

Frame input_pixels() {
    Frame pixels(size_t(W)*H*4);
    for(uint32_t y=0;y<H;y++)for(uint32_t x=0;x<W;x++) {
        const auto index=(size_t(y)*W+x)*4;
        const double dx=double(x)-75,dy=double(y)-89;
        const bool lamp=dx*dx+dy*dy < 19*19 ||
            (x>176&&x<211&&y>33&&y<67) ||
            (x>135&&x<145&&y>175&&y<218);
        pixels[index+0]=lamp?245:uint8_t((x*170)/W+16);
        pixels[index+1]=lamp?224:uint8_t((y*136)/H+18);
        pixels[index+2]=lamp?173:uint8_t(((x+y)*94)/(W+H)+22);
        pixels[index+3]=255;
    }
    return pixels;
}
Frame render(gs_effect_t* effect, gs_texture_t* input, gs_texture_t* noise,
             gs_texrender_t* target, gs_stagesurf_t* stage,
             float density,float refraction,float glint,
             float chromatic,float seed,float bead_size=1.f,float gravity_stretch=.3f) {
    auto set=[&](const char* name,float value) {
        auto *p=gs_effect_get_param_by_name(effect,name);
        if(!p) {
            std::cerr<<"FAIL: missing shader uniform "<<name<<"\n";
            return false;
        }
        gs_effect_set_float(p,value);
        return true;
    };
    auto *main_param=gs_effect_get_param_by_name(effect,"image");
    auto *noise_param=gs_effect_get_param_by_name(effect,"noise_tex");
    auto *uv_param=gs_effect_get_param_by_name(effect,"uv_pixel_interval");
    if(!main_param||!noise_param||!uv_param)return {};
    gs_effect_set_texture(main_param,input);
    gs_effect_set_texture(noise_param,noise);
    struct vec4 pixel_size{1.f/W,1.f/H,0.f,0.f};
    gs_effect_set_vec4(uv_param,&pixel_size);
    if(!set("wet_density",density)||!set("wet_size",bead_size)||
       !set("wet_refraction",refraction)||!set("wet_glint",glint)||
       !set("wet_stretch",gravity_stretch)||!set("wet_chromatic",chromatic)||
       !set("wet_seed",seed))return {};

    gs_texrender_reset(target);
    if(!gs_texrender_begin(target,W,H))return {};
    struct vec4 black{};
    gs_clear(GS_CLEAR_COLOR,&black,0.f,0);
    gs_blend_state_push();
    gs_reset_blend_state();
    gs_enable_blending(false);
    gs_blend_function(GS_BLEND_ONE,GS_BLEND_ZERO);
    gs_projection_push();
    gs_matrix_push();
    gs_matrix_identity();
    gs_ortho(0.f,float(W),0.f,float(H),-100.f,100.f);
    int pass_calls=0;
    gs_technique_t* tech=gs_effect_get_technique(effect,density<0?"Draw":"DrawWetLens");
    if(tech) {
        const size_t count=gs_technique_begin(tech);
        for(size_t i=0;i<count;++i) {
            if(gs_technique_begin_pass(tech,uint32_t(i))) {
                ++pass_calls;
                gs_draw_sprite(nullptr,0,W,H);
                gs_technique_end_pass(tech);
            }
        }
        gs_technique_end(tech);
    }
    if(pass_calls!=1)std::cerr<<"Wet Lens passes="<<pass_calls<<"\n";
    gs_matrix_pop();
    gs_projection_pop();
    gs_blend_state_pop();
    gs_texrender_end(target);
    auto* rendered=gs_texrender_get_texture(target);
    if(!rendered)return {};
    gs_stage_texture(stage,rendered);
    uint8_t* ptr=nullptr;uint32_t pitch=0;
    if(!gs_stagesurface_map(stage,&ptr,&pitch))return {};
    Frame result(size_t(W)*H*4);
    if(pitch<W*4)result.clear();
    else for(uint32_t y=0;y<H;y++)
        std::copy_n(ptr+size_t(y)*pitch,W*4,result.data()+size_t(y)*W*4);
    gs_stagesurface_unmap(stage);
    return result;
}
// Connected optical footprints measured from real OBS GPU readback.
// Exclude single-pixel noise; the goal is distinct small beads and larger
// pooled droplets, rather than identical stamped ovals.
std::vector<size_t> droplet_footprints(const Frame& clean,const Frame& refracted) {
    std::vector<size_t> footprints;
    if(clean.size()!=refracted.size()||clean.size()!=size_t(W)*H*4)return footprints;
    std::vector<uint8_t> changed(size_t(W)*H,0);
    for(size_t p=0;p<changed.size();++p) {
        size_t i=p*4;
        int delta=std::abs(int(clean[i])-int(refracted[i]))+
                  std::abs(int(clean[i+1])-int(refracted[i+1]))+
                  std::abs(int(clean[i+2])-int(refracted[i+2]));
        changed[p]=delta>12;
    }
    std::vector<size_t> queue;
    queue.reserve(size_t(W)*H);
    for(size_t p=0;p<changed.size();++p) {
        if(!changed[p])continue;
        queue.clear();
        queue.push_back(p);
        changed[p]=0;
        for(size_t k=0;k<queue.size();++k) {
            size_t q=queue[k];
            size_t x=q%W,y=q/W;
            const size_t neighbors[4]={q>0?q-1:q,q+1,q>=W?q-W:q,q+W};
            for(int n=0;n<4;++n) {
                if((n==0&&x==0)||(n==1&&x+1>=W)||
                   (n==2&&y==0)||(n==3&&y+1>=H))continue;
                size_t next=neighbors[n];
                if(changed[next]) {changed[next]=0;queue.push_back(next);}
            }
        }
        if(queue.size()>=5)footprints.push_back(queue.size());
    }
    return footprints;
}
size_t diff(const Frame& a,const Frame& b) {
    if(a.empty()||b.empty()||a.size()!=b.size())return 0;
    size_t changed=0;
    for(size_t i=0;i<a.size();i+=4)
        if(std::abs(int(a[i])-int(b[i]))+
           std::abs(int(a[i+1])-int(b[i+1]))+
           std::abs(int(a[i+2])-int(b[i+2]))>3)++changed;
    return changed;
}
}

int main(int argc,char** argv) {
    if(argc!=4) {
        std::cerr<<"usage: wet_lens_gpu shader.effect plugin.so plugin-data-dir\n";
        return 2;
    }
    if(!obs_startup("en-US",nullptr,nullptr))return 2;
    obs_video_info settings{};
    settings.graphics_module="libobs-opengl";
    settings.fps_num=30;settings.fps_den=1;
    settings.base_width=W;settings.base_height=H;
    settings.output_width=W;settings.output_height=H;
    settings.output_format=VIDEO_FORMAT_RGBA;
    settings.colorspace=VIDEO_CS_DEFAULT;
    settings.range=VIDEO_RANGE_DEFAULT;
    settings.scale_type=OBS_SCALE_DISABLE;
    if(obs_reset_video(&settings)!=OBS_VIDEO_SUCCESS) {
        std::cerr<<"FAIL: OBS graphics initialization\n";obs_shutdown();return 1;
    }
    // Open the compiled module in a separate headless OBS process. This
    // checks registration and the real property surface, not just shader math.
    obs_module_t* module=nullptr;
    const int opened=obs_open_module(&module,argv[2],argv[3]);
    if(opened!=MODULE_SUCCESS || !module || !obs_init_module(module)) {
        std::cerr<<"FAIL: staged EmuLensFX module did not initialize: "
                 <<opened<<"\n";
        obs_shutdown();
        return 1;
    }
    auto *props=obs_get_source_properties("emulens_fx");
    auto *mode=props?obs_properties_get(props,"mode"):nullptr;
    auto *group=props?obs_properties_get(props,"group_wet_lens"):nullptr;
    auto *coverage=props?obs_properties_get(props,"wet_density"):nullptr;
    bool registered=false;
    if(mode && group && coverage) {
        const auto size=obs_property_list_item_count(mode);
        for(size_t i=0;i<size;++i)
            if(obs_property_list_item_int(mode,i)==15)registered=true;
    }
    if(props)obs_properties_destroy(props);
    if(!registered) {
        std::cerr<<"FAIL: registered OBS Wet Lens mode or controls missing\n";
        obs_shutdown();
        return 1;
    }
    bool ok=false;
    obs_enter_graphics();
    auto previous_cull=gs_get_cull_mode();
    gs_set_cull_mode(GS_NEITHER);
    char* errors=nullptr;
    auto* effect=gs_effect_create_from_file(argv[1],&errors);
    if(errors) {
        std::cerr<<"shader diagnostics: "<<errors<<"\n";
        bfree(errors);
    }
    if(!effect) {
        std::cerr<<"FAIL: actual OBS shader compile failed\n";
    } else {
        auto *tech = gs_effect_get_technique(effect,"DrawWetLens");
        if(tech) {
            const size_t count=gs_technique_begin(tech);
            for(size_t i=0;i<count;++i) {
                if(gs_technique_begin_pass(tech,uint32_t(i)))
                    gs_technique_end_pass(tech);
            }
            gs_technique_end(tech);
        }
        auto data=input_pixels();
        const uint8_t* data_ptr=data.data();
        auto* image=gs_texture_create(W,H,GS_RGBA,1,&data_ptr,0);
        Frame noise_bytes(256u*256u);
        uint32_t rng=0x71ab53ed;
        for(auto& n:noise_bytes) {
            rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;
            n=uint8_t(rng&255);
        }
        const uint8_t* noise_ptr=noise_bytes.data();
        auto* noise=gs_texture_create(256,256,GS_R8,1,&noise_ptr,0);
        // Two different instance-noise textures must not alter the layout
        // when the same persisted wet_seed is used.
        Frame other_noise_bytes=noise_bytes;
        for(auto& n:other_noise_bytes)n=uint8_t(~n);
        const uint8_t* other_noise_ptr=other_noise_bytes.data();
        auto* other_noise=gs_texture_create(256,256,GS_R8,1,&other_noise_ptr,0);
        auto* target=gs_texrender_create(GS_RGBA,GS_ZS_NONE);
        auto* stage=gs_stagesurface_create(W,H,GS_RGBA);
        if(image&&noise&&other_noise&&target&&stage) {
            auto baseline=render(effect,image,noise,target,stage,-1,0,0,0,11);
            std::cout<<"baseline count="<<baseline.size()
                     <<" sample="<<(baseline.empty()?0:int(baseline[3]))<<"\n";
            auto clear=render(effect,image,noise,target,stage,0,0,0,0,11);
            auto neutral=render(effect,image,noise,target,stage,1,0,0,0,11);
            auto refracted=render(effect,image,noise,target,stage,1,.8,0,0,11);
            auto lit=render(effect,image,noise,target,stage,1,.8,1.5,0,11);
            auto spectrum=render(effect,image,noise,target,stage,1,.8,1.5,.8,11);
            auto reseeded=render(effect,image,noise,target,stage,1,.8,1.5,.8,77);
            auto same_seed_other_noise=render(effect,image,other_noise,target,stage,1,.8,1.5,.8,11);
            // Slider extremes must render through the real GPU shader path.
            auto max_beads=render(effect,image,noise,target,stage,1,.8,0,0,11,1.25f,.8f);
            size_t passthrough=diff(clear,neutral);
            const size_t clear_mismatch=diff(baseline,clear);
            size_t nonflat_pixels=0;
            for(size_t i=0;i+3<clear.size();i+=4)
                nonflat_pixels += int(clear[i])+int(clear[i+1])+
                                  int(clear[i+2])>75;
            std::cout<<"baseline-vs-off="<<clear_mismatch
                     <<" visible_source_pixels="<<nonflat_pixels<<"\n";
            size_t refracted_delta=diff(neutral,refracted);
            size_t lighting_delta=diff(refracted,lit);
            size_t spectral_delta=diff(lit,spectrum);
            size_t seed_delta=diff(spectrum,reseeded);
            size_t instance_noise_delta=diff(spectrum,same_seed_other_noise);
            const auto footprints=droplet_footprints(neutral,refracted);
            size_t tiny_beads=0,large_beads=0;
            for(size_t area:footprints) {
                tiny_beads+=area>=15&&area<=35;
                large_beads+=area>=70;
            }
            std::cout<<"wet_optical_footprints="<<footprints.size()
                     <<" small="<<tiny_beads<<" large="<<large_beads<<" areas=";
            for(size_t area:footprints)std::cout<<area<<",";
            std::cout<<"\n";
            size_t bad_alpha=0;
            for(auto a:{&clear,&neutral,&refracted,&lit,&spectrum,&reseeded,&max_beads})
                for(size_t i=3;i<a->size();i+=4)
                    bad_alpha += ((*a)[i]!=255);
            std::cout<<"off_vs_neutral="<<passthrough
                     <<" refracted_pixels="<<refracted_delta
                     <<" light_response_pixels="<<lighting_delta
                     <<" chromatic_pixels="<<spectral_delta
                     <<" seed_delta_pixels="<<seed_delta
                     <<" instance_noise_delta="<<instance_noise_delta
                     <<" unexpected_alpha="<<bad_alpha<<"\n";
            ok=clear.size()==size_t(W)*H*4 &&
               neutral.size()==clear.size()&&refracted.size()==clear.size()&&
               lit.size()==clear.size()&&spectrum.size()==clear.size()&&
               reseeded.size()==clear.size()&&
               clear_mismatch<5&&nonflat_pixels>30000&&
               passthrough<5&&refracted_delta>100&&lighting_delta>20&&
               spectral_delta>10&&seed_delta>100&&instance_noise_delta==0&&bad_alpha==0&&
               tiny_beads>=2&&large_beads>=2&&
               max_beads.size()==clear.size()&&diff(neutral,max_beads)>100;
        }
        if(stage)gs_stagesurface_destroy(stage);
        if(target)gs_texrender_destroy(target);
        if(other_noise)gs_texture_destroy(other_noise);
        if(noise)gs_texture_destroy(noise);
        if(image)gs_texture_destroy(image);
        gs_effect_destroy(effect);
    }
    gs_set_cull_mode(previous_cull);
    obs_leave_graphics();
    obs_shutdown();
    if(!ok) {
        std::cerr<<"FAIL: Wet Lens OBS rendered output / controls\n";
        return 1;
    }
    std::cout<<"PASS: staged OBS module registers Wet Lens; actual shader refracts, responds to captured light, preserves alpha\n";
}
