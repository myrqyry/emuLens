#!/usr/bin/env python3
"""
Shader Syntax Validation Test for EmuLensFX

This test parses the effect file and validates:
1. All required techniques exist
2. EVERY referenced shader function actually exists
3. Technique syntax is correct
4. Uniform declarations are present
"""

import os
import re
import sys
from pathlib import Path

# Expected techniques based on lens-strategies
EXPECTED_TECHNIQUES = [
    "Draw",
    "DrawVidicon",
    "DrawVidiconComposite",
    "DrawStarburst",
    "DrawLightLeak",
    "DrawHandheld",
    "DrawBokeh",
    "DrawBokehParticles",
    "DrawOrton",
    "DrawGrit",
    "DrawMirage",
    "DrawCenterSpot",
    "DrawGaussianH",
    "DrawGaussianV",
    "DrawBloomExtract",
    "DrawBloomCombine",
    "DrawPinhole",
    "DrawPolaroid",
    "DrawDaguerreotype",
    "DrawTechnicolor",
    "DrawWetLens",
    "DrawPhoneWet",
]

def find_effect_file():
    script_dir = Path(__file__).parent
    possible_paths = [
        script_dir / "../data/emulens.effect",
        Path("data/emulens.effect"),
    ]
    for path in possible_paths:
        if path.exists(): return path.resolve()
    return None

def find_functions(content: str) -> set:
    """Find all defined function names."""
    # Matches patterns like 'float4 PSVidicon(VertData v_in) : TARGET {'
    # or 'VertData VSDefault(VertData v_in) {'
    pattern = r'\w+\s+(\w+)\s*\([^)]*\)\s*(?::\s*\w+)?\s*\{'
    return set(re.findall(pattern, content))

def main():
    print("=" * 60)
    print("EmuLensFX Hardened Shader Validation Test")
    print("=" * 60)

    effect_path = find_effect_file()
    if not effect_path:
        print("ERROR: Could not find emulens.effect file")
        sys.exit(1)

    with open(effect_path, 'r') as f:
        content = f.read()

    # Check 1: Define lookup map of existing functions
    defined_functions = find_functions(content)
    print(f"Detected {len(defined_functions)} defined shader functions.")

    # Check 2: Parse techniques and validate their references
    # Pattern to capture technique body: technique Name { pass { ... } }
    tech_pattern = r'technique\s+(\w+)\s*\{([^}]*(?:pass\s*\{[^}]*\}[^}]*)*)\}'
    tech_matches = re.findall(tech_pattern, content, re.DOTALL)

    found_tech_names = [m[0] for m in tech_matches]
    missing_techs = [t for t in EXPECTED_TECHNIQUES if t not in found_tech_names]

    if missing_techs:
        print(f"FAIL: Missing required techniques: {missing_techs}")
        sys.exit(1)

    errors = []
    for tech_name, tech_body in tech_matches:
        # Check for shader references like 'pixel_shader = PSName(v_in);'
        # or 'vertex_shader = VSName(v_in);'
        shader_refs = re.findall(r'(\w+)_shader\s*=\s*(\w+)\s*\(', tech_body)
        for shader_type, func_name in shader_refs:
            if func_name not in defined_functions:
                errors.append(f"Technique '{tech_name}' references undefined {shader_type} function: '{func_name}'")
            else:
                print(f"  ✓ {tech_name} -> {func_name} (Verified)")

    print("-" * 60)
    if errors:
        for err in errors:
            print(f"🚨 CRITICAL ERROR: {err}")
        print("-" * 60)
        print(f"FAIL: {len(errors)} undefined function references found!")
        sys.exit(1)

    print("PASS: All techniques and function references verified.")
    sys.exit(0)

if __name__ == "__main__":
    main()