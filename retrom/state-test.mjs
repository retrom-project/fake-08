// SPDX-License-Identifier: MIT
import assert from 'node:assert/strict';
import {readFile} from 'node:fs/promises';
import {pathToFileURL} from 'node:url';
const file = process.argv[2];
const create = (await import(pathToFileURL(file))).default;
const code = `pico-8 cartridge // http://www.pico-8.com\nversion 42\n__lua__\nx=10\nfunction _update60() if btn(1) then x+=1 end end\nfunction _draw() cls(0) rectfill(x,20,x+5,25,8) end\n`;
async function instance() {
 const m = await create({wasmBinary:await readFile(file.replace(/mjs$/, 'wasm'))});
 const data = new TextEncoder().encode(code); const p = m._malloc(data.length);
 m.HEAPU8.set(data,p); assert.equal(m._retrom_load(p,data.length),1); m._free(p);
 return m;
}
const a = await instance();
for(let i=0;i<10;i++) assert.equal(a._retrom_step(1<<7),1);
const pointer=a._retrom_state(); const size=a._retrom_state_size();
assert.ok(pointer && size>1024 && size<4*1024*1024);
const state=a.HEAPU8.slice(pointer,pointer+size);
const b=await instance();const p=b._malloc(size);b.HEAPU8.set(state,p);
assert.equal(b._retrom_restore(p,size-1),0);
assert.equal(b._retrom_restore(p,size),1);
assert.equal(a._retrom_step(0),1);assert.equal(b._retrom_step(0),1);
function frame(m){return m.HEAPU8.slice(m._retrom_pixels(),m._retrom_pixels()+128*128*4);}
assert.deepEqual(frame(b),frame(a));
const previous=frame(b);assert.equal(b._retrom_step(1<<7),1);assert.notDeepEqual(frame(b),previous);
b.HEAPU8[p]=255;assert.equal(b._retrom_restore(p,size),0);
b._free(p);a._retrom_stop();b._retrom_stop();
console.log('FAKE-08 fresh-instance state, bounds, corruption and resumed input PASS');
