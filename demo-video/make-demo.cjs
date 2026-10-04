const fs=require('fs'),cp=require('child_process'),path=require('path');
const bin=path.resolve(__dirname,'../target/familiar-paint');
const captureBin=path.resolve(__dirname,'build/capture');
if(process.argv[2]) {fs.mkdirSync(path.resolve(process.argv[2]));process.chdir(path.resolve(process.argv[2]));} else process.chdir(__dirname);
for(const dir of ['batches','images','captures','logs'])fs.mkdirSync(dir,{recursive:true});
if(!fs.existsSync('paint')) fs.symlinkSync(bin,'paint');
const rect=(x,y,w,h,color)=>({op:'rect',from:[x,y],to:[x+w,y+h],color,filled:true,width:1});
const line=(a,b,color,width=3)=>({op:'line',from:a,to:b,color,width});
const text=(x,y,word,size,color='#243044')=>({op:'text',at:[x,y],text:word,size,color,font:'DejaVu Sans'});
const groups=[
 [{op:'new',width:1000,height:580,color:'#f4f0e7'},text(42,28,'A FAMILIAR PLACE TO CREATE.',18,'#68717a')],
 [rect(205,132,616,338,'#d8d4c9'),rect(185,112,616,338,'#c7c8c5'),rect(199,126,588,310,'#ffffff'),rect(199,126,588,43,'#183e92'),text(214,135,'untitled — Paint',20,'#ffffff'),rect(747,136,27,22,'#d5d5cf'),line([754,142],[767,152],'#243044',2),line([767,142],[754,152],'#243044',2)],
 [text(253,207,'WELCOME',72),text(258,294,'TO FAMILIAR PAINT',28,'#536878'),rect(260,355,465,3,'#dcded8')],
 ['#243044','#2352ac','#3e9180','#efc95b','#e99563','#c96772','#ffffff'].map((c,i)=>rect(260+i*67,378,44,28,c)),
 [{op:'stroke',points:[[854,249],[854,344],[878,322],[897,358],[913,349],[891,313],[925,313],[854,249]],color:'#243044',width:6},line([144,217],[116,197],'#e99563',6),line([136,250],[100,250],'#e99563',6),line([144,281],[117,302],'#e99563',6),text(250,493,'Familiar tools. A new pair of hands.',25)],
 [text(42,548,'DRAWN ENTIRELY THROUGH THE CLI',14,'#68717a'),text(766,548,'MAKE YOURSELF AT HOME.',13,'#68717a')]
];
function run(args){ const r=cp.spawnSync('./paint',args,{encoding:'utf8'});if(r.status!==0)throw Error(r.stdout+r.stderr);return JSON.parse(r.stdout); }
let all=[],manifest=[];
for(let i=0;i<groups.length;i++){
 const n=String(i+1).padStart(2,'0');all.push(...groups[i]);
 fs.writeFileSync(`batches/${n}.json`,JSON.stringify({version:1,commands:groups[i]},null,2));
 const args=['--render',`batches/${n}.json`,...(i?['--input',`images/${String(i).padStart(2,'0')}.png`]:[]),'--output',`images/${n}.png`];
 const result=run(args);fs.writeFileSync(`logs/${n}.json`,JSON.stringify(result,null,2));
 const capture=cp.spawnSync(captureBin,[`images/${n}.png`,`captures/${n}.png`],{encoding:'utf8'});
 if(capture.status!==0)throw Error(capture.stderr);
 const command='./paint '+args.join(' ');
 manifest.push({stage:n,command,result});
 fs.writeFileSync(`logs/${n}-caption.txt`,command+'\n'+`OK  |  ${result.width} x ${result.height}  |  PNG saved`);
}
fs.writeFileSync('welcome.json',JSON.stringify({version:1,commands:all},null,2));
const complete=run(['--render','welcome.json','--output','images/complete.png']);
const inspect=run(['--inspect','images/complete.png']);
fs.writeFileSync('logs/inspect.json',JSON.stringify(inspect,null,2));
const bad={version:1,commands:[line([0,0],[100,100],'red'),{op:'invalid-operation'}]};
fs.writeFileSync('batches/invalid.json',JSON.stringify(bad));
const rejected=cp.spawnSync('./paint',['--render','batches/invalid.json','--input','images/complete.png','--output','images/should-not-exist.png'],{encoding:'utf8'});
if(rejected.status!==1||fs.existsSync('images/should-not-exist.png'))throw Error('Invalid batch was not rejected atomically');
fs.writeFileSync('logs/rejected.json',rejected.stdout);
const overwrite=cp.spawnSync('./paint',['--render','welcome.json','--output','images/complete.png'],{encoding:'utf8'});
if(overwrite.status!==1)throw Error('Overwrite guard failed');
fs.writeFileSync('logs/overwrite-refused.json',overwrite.stdout);
fs.writeFileSync('logs/manifest.json',JSON.stringify(manifest,null,2));
console.log('Six CLI stages, inspection, invalid-batch rejection and overwrite guard passed.');
