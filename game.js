export const words = ['signal','rhythm','keyboard','timer','circuit','buffer','serial','interrupt','memory','voltage','crystal','register','embedded','challenge','frequency','hardware','program','system','switch','output','input','clock','green','yellow','red','pulse','speed','accuracy','board','typing'];
export function tier(remaining, total) { const ratio=remaining/total; return ratio>=.6?{color:'green',points:7,label:'極速反應'}:ratio>=.3?{color:'yellow',points:5,label:'穩定過關'}:{color:'red',points:3,label:'驚險壓線'}; }
export function validateWord(input, target) { return input.trim().toLowerCase()===target.toLowerCase(); }
export function parseDevice(line) {
 if(line==='READY TYPERUSH/1') return {type:'ready'};
 if(/^ACK (PING|STATE|OFF|LED)$/.test(line)) return {type:'ack',command:line.slice(4)};
 if(line.startsWith('ERR ')) return {type:'error',message:line.slice(4)};
 return null;
}
