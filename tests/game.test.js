import test from 'node:test';
import assert from 'node:assert/strict';
import {tier,validateWord,parseDevice,words} from '../game.js';
test('PDF scoring thresholds and speed independence',()=>{for(const total of [6,10,15]){assert.equal(tier(total*.6,total).points,7);assert.equal(tier(total*.599,total).points,5);assert.equal(tier(total*.3,total).points,5);assert.equal(tier(total*.299,total).points,3);}});
test('spelling accepts case and outer whitespace but rejects incorrect spelling',()=>{assert.ok(validateWord(' SIGNAL ','signal'));assert.ok(!validateWord('singal','signal'));assert.ok(!validateWord('','signal'));assert.ok(words.length>=20);});
test('device only acknowledges lights and never controls the game',()=>{assert.deepEqual(parseDevice('READY TYPERUSH/1'),{type:'ready'});assert.deepEqual(parseDevice('ACK PING'),{type:'ack',command:'PING'});for(const action of ['START','PAUSE','COLOR','SPEED'])assert.equal(parseDevice('BTN '+action),null);assert.deepEqual(parseDevice('ERR OVERFLOW'),{type:'error',message:'OVERFLOW'});});
