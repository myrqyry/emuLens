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
             float coverage,float refraction,float ribbons,
             float reach,float glow,float arcs,float seed,
             float angle=-5.f) {
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
    if(!set("phone_coverage",coverage)||!set("phone_scale",1.25f)||
       !set("phone_refraction",refraction)||!set("phone_ribbons",ribbons)||
       !set("phone_reach",reach)||!set("phone_glow",glow)||
       !set("phone_arcs",arcs)||!set("phone_angle",angle)||
       !set("phone_seed",seed))return {};

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
    gs_technique_t* tech=gs_effect_get_technique(effect,coverage<0?"Draw":"DrawPhoneWet");
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
    if(pass_calls!=1)std::cerr<<"Phone Wet Lens passes="<<pass_calls<<"\n";
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
        std::cerr<<"usage: phone_wet_gpu shader.effect plugin.so plugin-data-dir\n";
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
    auto *group=props?obs_properties_get(props,"group_phone_wet"):nullptr;
    auto *coverage_property=props?obs_properties_get(props,"phone_coverage"):nullptr;
    bool registered=false;
    if(mode && group && coverage_property) {
        const auto size=obs_property_list_item_count(mode);
        for(size_t i=0;i<size;++i)
            if(obs_property_list_item_int(mode,i)==16)registered=true;
    }
    if(props)obs_properties_destroy(props);
    if(!registered) {
        std::cerr<<"FAIL: registered OBS Phone Wet Lens mode or controls missing\n";
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
        auto *tech = gs_effect_get_technique(effect,"DrawPhoneWet");
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
        // Same persisted phone_seed must render identically with different
        // per-instance noise textures after reloading an OBS scene.
        Frame other_noise_bytes=noise_bytes;
        for(auto& n:other_noise_bytes)n=uint8_t(~n);
        const uint8_t* other_noise_ptr=other_noise_bytes.data();
        auto* other_noise=gs_texture_create(256,256,GS_R8,1,&other_noise_ptr,0);
        auto* target=gs_texrender_create(GS_RGBA,GS_ZS_NONE);
        auto* stage=gs_stagesurface_create(W,H,GS_RGBA);
        if(image&&noise&&other_noise&&target&&stage) {
            auto baseline=render(effect,image,noise,target,stage,-1,0,0,0,0,0,19);
            auto off=render(effect,image,noise,target,stage,0,0,0,0,0,0,19);
            auto neutral=render(effect,image,noise,target,stage,1,0,0,0,0,0,19);
            auto refracted=render(effect,image,noise,target,stage,1,.95,0,0,0,0,19);
            auto ribbons=render(effect,image,noise,target,stage,1,.95,1.75,.9,0,0,19);
            auto edges=render(effect,image,noise,target,stage,1,.95,1.75,.9,0,1.5,19);
            auto glowed=render(effect,image,noise,target,stage,1,.95,1.75,.9,1.0,1.5,19);
            auto reseeded=render(effect,image,noise,target,stage,1,.95,1.75,.9,1.0,1.5,77);
            auto same_seed_other_noise=render(effect,image,other_noise,target,stage,1,.95,1.75,.9,1.0,1.5,19);
            auto rotated=render(effect,image,noise,target,stage,1,.95,1.75,.9,1.0,1.5,19,63.f);
            Frame black_pixels(size_t(W)*H*4,0);
            for(size_t i=3;i<black_pixels.size();i+=4)black_pixels[i]=255;
            const uint8_t* black_ptr=black_pixels.data();
            auto* black_source=gs_texture_create(W,H,GS_RGBA,1,&black_ptr,0);
            auto dark=black_source?
                render(effect,black_source,noise,target,stage,1,.95,1.75,.9,1.0,1.5,19):
                Frame{};
            if(black_source)gs_texture_destroy(black_source);

            const auto baseline_mismatch=diff(baseline,off);
            const auto neutral_mismatch=diff(off,neutral);
            const auto refract_delta=diff(neutral,refracted);
            const auto ribbons_delta=diff(refracted,ribbons);
            const auto edges_delta=diff(ribbons,edges);
            const auto glow_delta=diff(edges,glowed);
            const auto seed_delta=diff(glowed,reseeded);
            const auto instance_noise_delta=diff(glowed,same_seed_other_noise);
            const auto angle_delta=diff(glowed,rotated);
            size_t spurious_light=0;
            for(size_t i=0;i+3<dark.size();i+=4) {
                spurious_light += dark[i] + dark[i+1] + dark[i+2];
            }
            size_t alpha_errors=0;
            for(const auto* frame:{&baseline,&off,&neutral,&refracted,
                                   &ribbons,&edges,&glowed,&reseeded,&rotated,&dark})
                for(size_t i=3;i<frame->size();i+=4)
                    alpha_errors+=((*frame)[i]!=255);
            std::cout<<"baseline_mismatch="<<baseline_mismatch
                     <<" neutral_mismatch="<<neutral_mismatch
                     <<" refraction="<<refract_delta
                     <<" light_ribbons="<<ribbons_delta
                     <<" contour_edges="<<edges_delta
                     <<" soft_glow="<<glow_delta
                     <<" seed="<<seed_delta
                     <<" instance_noise_delta="<<instance_noise_delta
                     <<" angle="<<angle_delta
                     <<" dark_spurious_light="<<spurious_light
                     <<" bad_alpha="<<alpha_errors<<"\n";
            ok=baseline.size()==size_t(W)*H*4 &&
               off.size()==baseline.size() && neutral.size()==baseline.size() &&
               refracted.size()==baseline.size() && ribbons.size()==baseline.size() &&
               edges.size()==baseline.size() && glowed.size()==baseline.size() &&
               reseeded.size()==baseline.size() &&
               rotated.size()==baseline.size() && dark.size()==baseline.size() &&
               baseline_mismatch<5 && neutral_mismatch<5 &&
               refract_delta>100 && ribbons_delta>20 && edges_delta>20 &&
               glow_delta>20 && seed_delta>100 && instance_noise_delta==0 && angle_delta>100 &&
               spurious_light==0 && alpha_errors==0;
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
        std::cerr<<"FAIL: Phone Wet Lens OBS rendered output / controls\n";
        return 1;
    }
    std::cout<<"PASS: staged OBS module registers Phone Wet Lens; refraction, light ribbons, water contours, glare and alpha verified\n";
}
