export class SerialLink {
 constructor({log=()=>{},status=()=>{}}={}){this.log=log;this.status=status;this.ready=false;this.queue=Promise.resolve();this.generation=0;}
 async connect(){
  if(!navigator.serial)throw Error('請使用桌面 Chrome／Edge，並以 HTTPS 或 localhost 開啟。');
  this.port=await navigator.serial.requestPort();
  try{
   await this.port.open({baudRate:115200,dataBits:8,stopBits:1,parity:'none',flowControl:'none'});
   this.generation++;
   this.reader=this.port.readable.getReader();this.writer=this.port.writable.getWriter();
   this.readTask=this.read();
   this.status('handshake');
   await this.command('HELLO');
   this.ready=true;this.status('ready');
  }catch(error){await this.close();throw error;}
 }
 command(text){
  const generation=this.generation;
  const run=async()=>{
   if(!this.writer||generation!==this.generation)throw Error('序列埠已關閉。');
   let timer;
   const confirmation=new Promise((resolve,reject)=>{this.pending={text,resolve,reject};timer=setTimeout(()=>reject(Error(`板子未確認 ${text}，請檢查新版韌體或按 RESET。`)),1500);});
   // 先註冊拒絕處理，避免寫入尚未完成時產生未處理的拒絕。
   confirmation.catch(()=>{});
   try{this.log('→ '+text);await this.writer.write(new TextEncoder().encode(text+'\n'));await confirmation;}
   finally{clearTimeout(timer);this.pending=null;}
  };
  const result=this.queue.then(run);this.queue=result.catch(()=>{});return result;
 }
 async read(){const decoder=new TextDecoder();let buffer='';
  try{while(true){const {value,done}=await this.reader.read();if(done)break;buffer+=decoder.decode(value,{stream:true});let split;while((split=buffer.indexOf('\n'))>=0){const line=buffer.slice(0,split).trim();buffer=buffer.slice(split+1);this.log('← '+line);
   if(line===`OK ${this.pending?.text}`)this.pending.resolve();
   else if(line.startsWith('ERR ')){const error=Error('板子回報：'+line);this.pending?.reject(error);this.status('error',error.message);}
  }if(buffer.length>1024)throw Error('收到過長序列資料。');}}
  catch(error){if(!this.closing)this.status('error',error.message);}
  finally{this.reader?.releaseLock();this.reader=null;if(!this.closing){this.pending?.reject(Error('板子已中斷連線。'));void this.close();}}
 }
 async close(){
  if(this.closing)return;this.closing=true;this.ready=false;this.generation++;
  this.pending?.reject(Error('連線已關閉。'));
  try{await this.reader?.cancel();await this.readTask;await this.queue;this.writer?.releaseLock();this.writer=null;await this.port?.close();}
  catch(error){this.log('關閉：'+error.message);}
  finally{this.port=null;this.closing=false;this.status('closed');}
 }
}
