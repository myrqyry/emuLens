#!/usr/bin/env python3
"""Wind must move the particle lattice and lookup together, not strand particles."""
from pathlib import Path
s=(Path(__file__).resolve().parent.parent/'data/emulens.effect').read_text()
segment=s.split('float4 PSBokehParticles(',1)[1].split('technique DrawBokehParticles',1)[0]
assert 'float2 wind_offset = ' in segment, 'missing shared wind offset'
assert 'frac(v_in.uv - wind_offset)' in segment, 'grid lookup must follow wind'
assert 'pos += wind_offset;' in segment, 'particles must use the same displacement'
assert 'pos.x += part_wind_x * part_time * 0.1;' not in segment
assert 'pos.y += part_wind_y * part_time * 0.1;' not in segment
print('PASS: particle wind moves sampling lattice and particles together')
