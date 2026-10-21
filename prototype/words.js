// 词库与学段定义。小学 + 初中用 ECDICT 生成的真实词库（没有时退回下面 100 个手写词），其余位置是样例词占位，
// 让时间快进能跑完整条成长曲线。固件里这部分由打包脚本从词表生成。
'use strict';

const FALLBACK_WORDS = `
hello|həˈləʊ|你好
goodbye|ˌɡʊdˈbaɪ|再见
yes|jes|是；对
no|nəʊ|不；没有
thanks|θæŋks|谢谢
please|pliːz|请
sorry|ˈsɒri|对不起
name|neɪm|名字
I|aɪ|我
you|juː|你；你们
he|hiː|他
she|ʃiː|她
we|wiː|我们
they|ðeɪ|他们
boy|bɔɪ|男孩
girl|ɡɜːl|女孩
friend|frend|朋友
teacher|ˈtiːtʃə|老师
student|ˈstjuːdnt|学生
school|skuːl|学校
class|klɑːs|班级；课
book|bʊk|书
pen|pen|钢笔
pencil|ˈpensl|铅笔
ruler|ˈruːlə|尺子
bag|bæɡ|包；书包
desk|desk|书桌
chair|tʃeə|椅子
door|dɔː|门
window|ˈwɪndəʊ|窗户
red|red|红色
blue|bluː|蓝色
green|ɡriːn|绿色
yellow|ˈjeləʊ|黄色
black|blæk|黑色
white|waɪt|白色
one|wʌn|一
two|tuː|二
three|θriː|三
four|fɔː|四
five|faɪv|五
six|sɪks|六
seven|ˈsevn|七
eight|eɪt|八
nine|naɪn|九
ten|ten|十
big|bɪɡ|大的
small|smɔːl|小的
good|ɡʊd|好的
happy|ˈhæpi|快乐的
cat|kæt|猫
dog|dɒɡ|狗
bird|bɜːd|鸟
fish|fɪʃ|鱼
duck|dʌk|鸭子
pig|pɪɡ|猪
rabbit|ˈræbɪt|兔子
panda|ˈpændə|熊猫
monkey|ˈmʌŋki|猴子
tiger|ˈtaɪɡə|老虎
elephant|ˈelɪfənt|大象
mother|ˈmʌðə|母亲
father|ˈfɑːðə|父亲
sister|ˈsɪstə|姐妹
brother|ˈbrʌðə|兄弟
family|ˈfæməli|家庭
head|hed|头
face|feɪs|脸
eye|aɪ|眼睛
ear|ɪə|耳朵
nose|nəʊz|鼻子
mouth|maʊθ|嘴
hand|hænd|手
foot|fʊt|脚
apple|ˈæpl|苹果
banana|bəˈnɑːnə|香蕉
orange|ˈɒrɪndʒ|橙子
egg|eɡ|鸡蛋
milk|mɪlk|牛奶
water|ˈwɔːtə|水
rice|raɪs|米饭
bread|bred|面包
cake|keɪk|蛋糕
juice|dʒuːs|果汁
sun|sʌn|太阳
moon|muːn|月亮
star|stɑː|星星
tree|triː|树
flower|ˈflaʊə|花
run|rʌn|跑
jump|dʒʌmp|跳
swim|swɪm|游泳
sing|sɪŋ|唱歌
dance|dɑːns|跳舞
read|riːd|读
write|raɪt|写
like|laɪk|喜欢
play|pleɪ|玩
eat|iːt|吃
sleep|sliːp|睡觉`.trim().split('\n').map(l => { const [w, p, m] = l.split('|'); return { w, p, pos: '', m, real: true }; });
// 有 tools/build_wordlist.py 生成的词库（assets/words_data.js，小学 + 初中 1600 词）就用它，上面 100 词只是兜底
const REAL_WORDS = window.NABO_WORDS ? window.NABO_WORDS.map(([w, p, pos, m]) => ({ w, p, pos, m, real: true })) : FALLBACK_WORDS;

const UNIT_SIZE = 50;

// 学段：end 是累计词数（不含）
const STAGES = [
  { id: 'primary', name: '小学', region: '中国', end: 700, outfit: '黄色背心 + 书包', sprite: '豆豆',
    cities: '北京 上海 西安 成都 杭州 广州 桂林 拉萨 哈尔滨 厦门 重庆 昆明 青岛 敦煌' },
  { id: 'junior', name: '初中', region: '亚洲', end: 1600, outfit: '蓝白运动校服', sprite: '泡泡',
    cities: '东京 首尔 曼谷 新加坡 河内 吉隆坡 马尼拉 雅加达 加德满都 新德里 孟买 科伦坡 乌兰巴托 大阪 京都 清迈 巴厘岛 迪拜' },
  { id: 'senior', name: '高中', region: '欧洲', end: 3500, outfit: '藏青校服 + 领带', sprite: '星星',
    cities: '伦敦 巴黎 罗马 柏林 马德里 雅典 维也纳 布拉格 阿姆斯特丹 布鲁塞尔 苏黎世 日内瓦 威尼斯 佛罗伦萨 米兰 巴塞罗那 里斯本 都柏林 爱丁堡 牛津 剑桥 哥本哈根 斯德哥尔摩 奥斯陆 赫尔辛基 雷克雅未克 华沙 布达佩斯 慕尼黑 汉堡 莫斯科 圣彼得堡 伊斯坦布尔 尼斯 里昂 萨尔茨堡 克拉科夫 塞维利亚' },
  { id: 'cet4', name: '四级', region: '美洲', end: 4500, outfit: '灰色卫衣 + 耳机', sprite: '火火',
    cities: '纽约 波士顿 华盛顿 芝加哥 旧金山 洛杉矶 西雅图 迈阿密 新奥尔良 多伦多 温哥华 蒙特利尔 墨西哥城 哈瓦那 利马 库斯科 里约 布宜诺斯艾利斯 圣地亚哥 乌斯怀亚' },
  { id: 'cet6', name: '六级', region: '大洋洲和非洲', end: 6000, outfit: '白衬衫 + 背包', sprite: '浪浪',
    cities: '悉尼 墨尔本 布里斯班 珀斯 奥克兰 皇后镇 斐济 塔希提 开罗 卢克索 马拉喀什 卡萨布兰卡 内罗毕 桑给巴尔 开普敦 约翰内斯堡 维多利亚瀑布 达累斯萨拉姆 亚的斯亚贝巴 拉各斯 阿克拉 达喀尔 马达加斯加 毛里求斯 塞舌尔 突尼斯 乞力马扎罗 纳米布 奥卡万戈 霍巴特' },
  { id: 'adv', name: '进阶', region: '世界奇观', end: 7500, outfit: '风衣 + 相机', sprite: '极光',
    cities: '南极 北极 格陵兰 阿拉斯加 加拉帕戈斯 复活节岛 马丘比丘 撒哈拉 喜马拉雅 珠峰 亚马孙 大堡礁 黄石 大峡谷 尼亚加拉 黄刀镇 冰岛 死海 佩特拉 吴哥窟 婆罗浮屠 巴拉望 帕劳 马尔代夫 圣托里尼 少女峰 峡湾 富士山 乌尤尼 空间站' },
].map((s, i, arr) => ({ ...s, idx: i, start: i ? arr[i - 1].end : 0, cities: s.cities.split(' ') }));

const TOTAL_WORDS = STAGES[STAGES.length - 1].end;
const TOTAL_UNITS = TOTAL_WORDS / UNIT_SIZE;

// 样例词：英文用编号，释义标明是占位
const WORDS = Array.from({ length: TOTAL_WORDS }, (_, i) =>
  REAL_WORDS[i] || { w: 'sample' + String(i + 1).padStart(4, '0'), p: '', m: '样例词 ' + (i + 1), real: false });

function stageOf(wordIdx) { return STAGES.find(s => wordIdx < s.end) || STAGES[STAGES.length - 1]; }
function unitOf(wordIdx) { return Math.floor(wordIdx / UNIT_SIZE); }
function unitInfo(u) {
  const st = stageOf(u * UNIT_SIZE);
  const k = u - st.start / UNIT_SIZE;
  return { stage: st, k, city: st.cities[k] || `第 ${k + 1} 站` };
}

// 书桌道具（按已掌握词数解锁）与盲盒摆件
const PROPS = [
  { id: 'pencilcase', name: '铅笔盒', need: 0 }, { id: 'clock', name: '闹钟', need: 50 },
  { id: 'lamp', name: '台灯', need: 150 }, { id: 'plant', name: '绿植', need: 300 },
  { id: 'globe', name: '地球仪', need: 500 }, { id: 'shelf', name: '书架', need: 800 },
  { id: 'cert', name: '奖状', need: 1100 }, { id: 'phones', name: '耳机', need: 1600 },
  { id: 'laptop', name: '笔记本电脑', need: 2500 }, { id: 'mug', name: '咖啡杯', need: 3500 },
  { id: 'map', name: '世界地图', need: 4500 }, { id: 'trophy', name: '奖杯', need: 6000 },
];
const BOX_TOYS = ['小黄鸭', '小火箭', '仙人掌', '雪花球', '魔方', '小恐龙', '纸飞机', '沙漏', '贝壳', '风车'];

// 科举段位
const RANKS = [[0, '童生'], [1100, '秀才'], [1200, '举人'], [1350, '贡士'], [1500, '进士'], [1700, '探花'], [1850, '榜眼'], [2000, '状元']];
function rankOf(r) { let n = RANKS[0][1]; for (const [t, s] of RANKS) if (r >= t) n = s; return n; }
