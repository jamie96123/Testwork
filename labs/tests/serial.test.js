import test from 'node:test';
import assert from 'node:assert/strict';
import {SerialLink} from '../serial.js';
test('missing HELLO acknowledgement retains open port for reset diagnostics and explicit retry',async()=>{
 let controller,respond=false,closed=0;const statuses=[];const lines=[];
 const port={readable:new ReadableStream({start(c){controller=c;}}),writable:new WritableStream({write(bytes){const command=new TextDecoder().decode(bytes).trim();if(respond)controller.enqueue(new TextEncoder().encode('OK '+command+'\r\n'));}}),async open(){},async close(){closed++;}};
 const original=Object.getOwnPropertyDescriptor(globalThis,'navigator');
 Object.defineProperty(globalThis,'navigator',{configurable:true,value:{serial:{requestPort:async()=>port}}});
 const link=new SerialLink({status:(...args)=>statuses.push(args),log:line=>lines.push(line)});
 try{await link.connect();assert.equal(link.ready,false);assert.equal(closed,0);assert.ok(statuses.some(([type])=>type==='waiting'));assert.ok(!statuses.some(([type])=>type==='ready'));
  controller.enqueue(new TextEncoder().encode('HELLO WORLD\r\n'));await new Promise(r=>setTimeout(r,0));assert.ok(statuses.some(([type])=>type==='boot'));assert.ok(lines.includes('← HELLO WORLD'));
  respond=true;assert.equal(await link.verify(),true);assert.equal(link.ready,true);assert.ok(statuses.some(([type])=>type==='ready'));
 }finally{await link.close();if(original)Object.defineProperty(globalThis,'navigator',original);else delete globalThis.navigator;}
 assert.equal(closed,1);
});
