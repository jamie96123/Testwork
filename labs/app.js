import {Game,lightPlan} from './game.js';
import {SerialLink} from './serial.js';
const $=id=>document.getElementById(id);
const game=new Game();
let manual=null, lastRound=0, desiredKey='', revision=0, syncing=false, syncingPromise;
const log=text=>{const el=$('log');el.textContent=(el.textContent+'\n'+new Date().toLocaleTimeString()+' '+text).split('\n').slice(-100).join('\n');el.scrollTop=el.scrollHeight;};
const link=new SerialLink({log,status(type,message){
 if(type==='ready'){$('retry').hidden=true;$('connection-label').textContent='STM32 已確認連線';$('connection-detail').textContent='LPUART1 · 115200 / 8N1 · 新版教材韌體';$('connection-dot').classList.add('connected');$('connect').hidden=true;$('disconnect').hidden=false;desiredKey='';render();}
 if(type==='waiting'){$('connection-label').textContent='序列埠已開啟，韌體尚未確認';$('connection-detail').textContent=message;$('connect').hidden=true;$('disconnect').hidden=false;$('retry').hidden=false;$('connection-dot').classList.remove('connected');document.querySelector('.console').open=true;}
 if(type==='boot'){$('connection-detail').textContent='已收到 HELLO WORLD：板子 → 電腦正常。請按「重新確認」檢查另一方向。';}
 if(type==='handshake'){$('connection-label').textContent='序列埠已開啟，確認韌體中…';}
 if(type==='error'){$('hardware-status').textContent=message;$('connection-label').textContent='板子通訊異常';$('connection-dot').classList.remove('connected');}
 if(type==='closed'){$('retry').hidden=true;$('connection-label').textContent='模擬模式';$('connection-detail').textContent='序列埠已關閉，遊戲可繼續；實體燈光狀態未確認。';$('connection-dot').classList.remove('connected');$('connect').hidden=false;$('connect').disabled=false;$('disconnect').hidden=true;$('lamp-source').textContent='● SIMULATED';$('hardware-status').textContent='目前僅顯示模擬燈光；若板子斷線，請按 RESET 關燈。';desiredKey='';}
}});
function lightState(){const color=manual??game.color;return {color,breathe:color==='green'&&$('breathe').checked&&game.state!=='paused'};}
function showLight({color,breathe}){document.querySelectorAll('.lamp').forEach(el=>el.classList.toggle('active',el.dataset.color===color));const traffic=document.querySelector('.traffic');traffic.classList.toggle('breathing',breathe);traffic.setAttribute('aria-label',color==='off'?'目前燈光全關':`${{green:'綠',yellow:'黃',red:'紅'}[color]}燈${breathe?'呼吸':'恆亮'}`);}
function syncLight(){const desired=lightState();showLight(desired);const key=JSON.stringify(desired);if(desiredKey===key)return;desiredKey=key;revision++;if(!link.ready)return;
 $('hardware-status').textContent='正在送出燈光指令，等待板子確認…';
 if(!syncing){syncingPromise=drainLights();}
}
async function drainLights(){syncing=true;try{while(link.ready){const version=revision;const desired=lightState();for(const command of lightPlan(desired.color,desired.breathe))await link.command(command);if(version===revision){$('hardware-status').textContent=`板子已確認：${desired.color==='off'?'三燈全關':{green:'綠燈',yellow:'黃燈',red:'紅燈'}[desired.color]+(desired.breathe?'呼吸效果':'恆亮')}。`;$('lamp-source').textContent='● BOARD ACK';break;}}}catch(error){log(error.message);$('hardware-status').textContent='輸出未確認：'+error.message;$('lamp-source').textContent='● UNCONFIRMED';await link.close();}finally{syncing=false;}}
function render(){
 $('score').textContent=String(game.score).padStart(3,'0');$('accuracy').textContent=game.accuracy===null?'—':game.accuracy+'%';$('streak').textContent=String(game.streak).padStart(2,'0');$('seconds').textContent=game.remaining.toFixed(1);$('word').textContent=game.state==='idle'?'ready?':game.word;$('phase').textContent=({idle:'READY TO PLAY',running:'IN THE FLOW',paused:'PAUSED',finished:'CHALLENGE COMPLETE'})[game.state];$('word-label').textContent=game.state==='finished'?'YOU FOUND YOUR FLOW':'TYPE THE WORD. KEEP THE RHYTHM.';$('progress').style.width=Math.max(0,game.remaining/game.total*100)+'%';$('progress').style.background=({green:'var(--green)',yellow:'var(--yellow)',red:'var(--red)',off:'var(--lime)'})[game.color];$('round').textContent=String(game.round).padStart(2,'0')+' / 10';$('feedback').textContent=game.result;$('answer').disabled=$('submit').disabled=game.state!=='running';$('pause').disabled=!['running','paused'].includes(game.state);$('pause').textContent=game.state==='paused'?'▶ 繼續':'Ⅱ 暫停';$('start').textContent=game.state==='idle'?'▶ 開始挑戰':'↻ 重新挑戰';$('speed').disabled=game.state==='running';document.querySelectorAll('[data-light]').forEach(el=>el.disabled=game.state==='running');if(lastRound!==game.round){$('answer').value='';lastRound=game.round;}syncLight();
}
$('start').onclick=()=>{manual=null;game.start();render();$('answer').focus();};
$('pause').onclick=()=>{manual=null;if(game.state==='running')game.pause();else game.resume();render();if(game.state==='running')$('answer').focus();};
$('reset').onclick=()=>{manual=null;game.reset();$('answer').value='';render();};
$('speed').onchange=()=>{game.setLimit(Number($('speed').value));if(game.state==='paused')game.result='下一題起使用新的限時；目前倒數保持不變。';render();};
$('answer-form').onsubmit=event=>{event.preventDefault();manual=null;const success=game.submit($('answer').value);if(success)$('answer').value='';render();};
setInterval(()=>{if(game.state==='running'){game.update();render();}},50);
document.querySelectorAll('[data-light]').forEach(button=>button.onclick=()=>{if(game.state==='running')return;manual=button.dataset.light;render();});
$('breathe').onchange=()=>{render();};
$('connect').onclick=async()=>{$('connect').disabled=true;try{await link.connect();}catch(error){$('connection-label').textContent='無法完成連線';$('connection-detail').textContent=error.message;log(error.message);$('connect').disabled=false;}};
$('retry').onclick=async()=>{$('retry').disabled=true;try{await link.verify();}catch(error){$('connection-detail').textContent=error.message;log(error.message);}finally{$('retry').disabled=false;}};
$('disconnect').onclick=async()=>{$('disconnect').disabled=true;try{manual='off';syncLight();await syncingPromise;await link.close();manual=null;render();}finally{$('disconnect').disabled=false;}};
$('clear-log').onclick=()=>{$('log').textContent='紀錄已清除。';};
// 切換分頁時凍結遊戲，避免背景分頁計時節流導致題目跳過。
document.addEventListener('visibilitychange',()=>{if(document.hidden&&game.state==='running'){manual=null;game.pause();render();}});
render();
