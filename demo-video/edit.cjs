const fs=require('fs'),cp=require('child_process');process.chdir(process.argv[2] || __dirname);
const shots=[
 ['06',3,'A welcome card. Painted through the CLI.'],
 ['01',3,'01 / Start with a blank canvas'],
 ['02',3,'02 / Build a familiar little window'],
 ['03',3,'03 / Write the welcome'],
 ['04',3,'04 / Add a splash of colour'],
 ['05',3,'05 / Give it a little personality'],
 ['06',5,'06 / Save the finished welcome card']
];
fs.mkdirSync('render',{recursive:true});
const disclosure='Real Qt app captures of CLI-generated files. Staged playback; live socket control not demonstrated.';
fs.writeFileSync('logs/disclosure.txt',disclosure);
let concat='';
for(let i=0;i<shots.length;i++){
 const [stage,duration,title]=shots[i];fs.writeFileSync(`logs/title-${i}.txt`,title);
 const cap=i===0?'logs/hook.txt':`logs/${stage}-caption.txt`;
 if(i===0)fs.writeFileSync(cap,'Six JSON batches. One welcome card.\nNative C++ / Qt Paint. No image-generation service.');
 const font='/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf',mono='/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf';
 const filter=`pad=1280:1024:0:60:color=0x10171e,drawtext=fontfile=${font}:textfile=logs/title-${i}.txt:fontsize=26:fontcolor=0xf4f0e7:x=24:y=16,drawtext=fontfile=${mono}:textfile=${cap}:fontsize=18:fontcolor=0xb9e5ce:x=24:y=921:line_spacing=8,drawtext=fontfile=${font}:textfile=logs/disclosure.txt:fontsize=14:fontcolor=0x9ea8b2:x=24:y=997`;
 const args=['-hide_banner','-loglevel','error','-y','-loop','1','-framerate','30','-i',`captures/${stage}.png`,'-t',String(duration),'-vf',filter,'-c:v','libx264','-preset','fast','-crf','18','-pix_fmt','yuv420p',`render/shot-${i}.mp4`];
 cp.execFileSync('ffmpeg',args);concat+=`file 'shot-${i}.mp4'\n`;
}
fs.writeFileSync('render/concat.txt',concat);
cp.execFileSync('ffmpeg',['-hide_banner','-loglevel','error','-y','-f','concat','-safe','0','-i','render/concat.txt','-c','copy','-movflags','+faststart','familiar-paint-welcome.mp4']);
fs.writeFileSync('EDIT_DECISIONS.json',JSON.stringify({silent:true,format:'1280x1024 30fps H264',shots,disclosure},null,2));
function pixels(file){return cp.execFileSync('ffmpeg',['-v','error','-i',file,'-pix_fmt','rgba','-f','md5','-'],{encoding:'utf8'}).trim();}
if(pixels('images/06.png')!==pixels('images/complete.png'))throw Error('Staged and single-batch pixels differ');
fs.writeFileSync('logs/pixel-equality.txt','Staged vs single-batch render: pixel-identical\n'+pixels('images/06.png')+'\n');
console.log('Video rendered; staged and single-batch outputs are pixel-identical.');
