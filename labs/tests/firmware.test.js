import test from 'node:test';
import {execFileSync} from 'node:child_process';
import {mkdtempSync,rmSync} from 'node:fs';
import {tmpdir} from 'node:os';
import {join} from 'node:path';
import {fileURLToPath} from 'node:url';
test('actual firmware application logic with HAL test doubles (host, not an ARM build)',()=>{const dir=mkdtempSync(join(tmpdir(),'typerush-hal-'));const source=fileURLToPath(new URL('./firmware_test.c',import.meta.url));const includes=fileURLToPath(new URL('./hal',import.meta.url));try{execFileSync('gcc',['-std=c11','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-I',includes,source,'-o',join(dir,'test')],{stdio:'pipe'});console.log(execFileSync(join(dir,'test'),{encoding:'utf8'}).trim());}finally{rmSync(dir,{recursive:true,force:true});}});
