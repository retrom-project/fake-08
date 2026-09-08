// SPDX-License-Identifier: MIT
import assert from 'node:assert/strict';
import {readFile} from 'node:fs/promises';
import {pathToFileURL} from 'node:url';
const file=process.argv[2],create=(await import(pathToFileURL(file))).default;
const functions=Array.from({length:180},(_,i)=>`f[${i+1}]=function() local n=${i%100} return function() return n+x end end`).join('\n');
const cart=new TextEncoder().encode(`pico-8 cartridge // http://www.pico-8.com\nversion 42\n__lua__\nx=10 f={}\n${functions}\nfunction _update60() if btn(1) then x+=1 end end\nfunction _draw() cls(0) for i=1,180 do pset(i%128,flr(i/128)+20,f[i]()()%16) end end\n`);
async function instance(){const m=await create({wasmBinary:await readFile(file.replace(/mjs$/,'wasm'))});const p=m._malloc(cart.length);m.HEAPU8.set(cart,p);assert.equal(m._retrom_load(p,cart.length),1);m._free(p);return m;}
const a=await instance();for(let i=0;i<45;i++)assert.equal(a._retrom_step(128),1);
const ptr=a._retrom_state(),size=a._retrom_state_size();assert.ok(ptr&&size>0);const state=a.HEAPU8.slice(ptr,ptr+size);
const b=await instance(),p=b._malloc(size);b.HEAPU8.set(state,p);assert.equal(b._retrom_restore(p,size),1);b._free(p);
for(let i=0;i<180;i++){const mask=i<60?128:0;assert.equal(a._retrom_step(mask),1);assert.equal(b._retrom_step(mask),1);assert.deepEqual(b.HEAPU8.slice(b._retrom_pixels(),b._retrom_pixels()+65536),a.HEAPU8.slice(a._retrom_pixels(),a._retrom_pixels()+65536),'closure graph and GC continuation frame '+i);}
a._retrom_stop();b._retrom_stop();console.log('FAKE-08 large closure graph restore and subsequent GC PASS');
