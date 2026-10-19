export const WORDS = ['clock','signal','buffer','timer','green','yellow','circuit','serial','output','rhythm','system','keyboard','memory','voltage','register','interrupt','hardware','challenge','program','accuracy'];
export function tier(remaining,total) {
 const ratio=remaining/total;
 return ratio>=.6?{color:'green',points:7}:ratio>=.3?{color:'yellow',points:5}:{color:'red',points:3};
}
export class Game {
 constructor(now=()=>performance.now(),random=Math.random){this.now=now;this.random=random;this.limit=10;this.reset();}
 reset(){this.state='idle';this.score=0;this.correct=0;this.completed=0;this.streak=0;this.round=0;this.remaining=this.limit;this.total=this.limit;this.word='ready';this.result='準備好，讓下一次敲擊發光。';}
 setLimit(value){if(![6,10,15].includes(value))return;this.limit=value;if(this.state==='idle'){this.remaining=this.total=value;}}
 start(){this.reset();this.state='running';this.next();this.result='輸入畫面上的單字，按 Enter 送出。';}
 next(){if(this.completed===10){this.state='finished';this.word='complete';this.result=`挑戰完成！答對 ${this.correct} / 10 題，獲得 ${this.score} 分。`;return;}
 let choices=WORDS.filter(w=>w!==this.word);this.word=choices[Math.floor(this.random()*choices.length)];this.round=this.completed+1;this.total=this.remaining=this.limit;this.deadline=this.now()+this.total*1000;}
 update(){if(this.state!=='running')return;this.remaining=Math.max(0,(this.deadline-this.now())/1000);if(this.remaining<=0){this.completed++;this.streak=0;this.result=`時間到，上一題是 ${this.word}。`;this.next();}}
 submit(input){if(this.state!=='running')return false;const oldRound=this.round;this.update();if(this.state!=='running'||this.round!==oldRound)return false;if(input.trim().toLowerCase()!==this.word){this.result='拼字還差一點，修正後再送出。';return false;}const points=tier(this.remaining,this.total).points;this.score+=points;this.correct++;this.completed++;this.streak++;this.result=`${this.word} 拼字正確！+${points} 分。`;this.next();return true;}
 pause(){if(this.state!=='running')return;this.update();if(this.state==='running'){this.state='paused';this.result='已暫停，倒數與作答都已凍結。';}}
 resume(){if(this.state!=='paused')return;this.deadline=this.now()+this.remaining*1000;this.state='running';this.result='繼續挑戰，找回你的節奏。';}
 get color(){return this.state==='idle'||this.state==='finished'?'off':tier(this.remaining,this.total).color;}
 get accuracy(){return this.completed?Math.round(this.correct/this.completed*100):null;}
}
/* 保留 Lab4 四條指令；其餘色彩／呼吸指令為同樣字串判斷的應用擴充。 */
export function lightPlan(color,breathe=false){return [
 'BREATHE OFF','GREEN OFF','YELLOW OFF','RED OFF',
 ...(color==='off'?[]:[`${color.toUpperCase()} ON`]),
 ...(color==='green'&&breathe?['BREATHE ON']:[])
];}
