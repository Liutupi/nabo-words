#include "nw_data.h"

#include <string.h>

static const char *const C0[] = {"北京", "上海", "西安", "成都", "杭州", "广州", "桂林", "拉萨", "哈尔滨", "厦门",
                                 "重庆", "昆明", "青岛", "敦煌"};
static const char *const C1[] = {"东京", "首尔", "曼谷", "新加坡", "河内", "吉隆坡", "马尼拉", "雅加达", "加德满都",
                                 "新德里", "孟买", "科伦坡", "乌兰巴托", "大阪", "京都", "清迈", "巴厘岛", "迪拜"};
static const char *const C2[] = {"伦敦", "巴黎", "罗马", "柏林", "马德里", "雅典", "维也纳", "布拉格", "阿姆斯特丹",
                                 "布鲁塞尔", "苏黎世", "日内瓦", "威尼斯", "佛罗伦萨", "米兰", "巴塞罗那", "里斯本",
                                 "都柏林", "爱丁堡", "牛津", "剑桥", "哥本哈根", "斯德哥尔摩", "奥斯陆", "赫尔辛基",
                                 "雷克雅未克", "华沙", "布达佩斯", "慕尼黑", "汉堡", "莫斯科", "圣彼得堡", "伊斯坦布尔",
                                 "尼斯", "里昂", "萨尔茨堡", "克拉科夫", "塞维利亚"};
static const char *const C3[] = {"纽约", "波士顿", "华盛顿", "芝加哥", "旧金山", "洛杉矶", "西雅图", "迈阿密",
                                 "新奥尔良", "多伦多", "温哥华", "蒙特利尔", "墨西哥城", "哈瓦那", "利马", "库斯科",
                                 "里约", "布宜诺斯艾利斯", "圣地亚哥", "乌斯怀亚"};
static const char *const C4[] = {"悉尼", "墨尔本", "布里斯班", "珀斯", "奥克兰", "皇后镇", "斐济", "塔希提", "开罗",
                                 "卢克索", "马拉喀什", "卡萨布兰卡", "内罗毕", "桑给巴尔", "开普敦", "约翰内斯堡",
                                 "维多利亚瀑布", "达累斯萨拉姆", "亚的斯亚贝巴", "拉各斯", "阿克拉", "达喀尔",
                                 "马达加斯加", "毛里求斯", "塞舌尔", "突尼斯", "乞力马扎罗", "纳米布", "奥卡万戈", "霍巴特"};
static const char *const C5[] = {"南极", "北极", "格陵兰", "阿拉斯加", "加拉帕戈斯", "复活节岛", "马丘比丘", "撒哈拉",
                                 "喜马拉雅", "珠峰", "亚马孙", "大堡礁", "黄石", "大峡谷", "尼亚加拉", "黄刀镇", "冰岛",
                                 "死海", "佩特拉", "吴哥窟", "婆罗浮屠", "巴拉望", "帕劳", "马尔代夫", "圣托里尼",
                                 "少女峰", "峡湾", "富士山", "乌尤尼", "空间站"};

#define N(a) (uint8_t)(sizeof(a) / sizeof((a)[0]))
const nw_stage_info_t NW_STAGE_INFO[NW_STAGES] = {
    {"小学", "中国", C0, N(C0)},       {"初中", "亚洲", C1, N(C1)},         {"高中", "欧洲", C2, N(C2)},
    {"四级", "美洲", C3, N(C3)},       {"六级", "大洋洲和非洲", C4, N(C4)}, {"进阶", "世界奇观", C5, N(C5)},
};

const char *const NW_STAGE_KEYS[NW_LOOKS] = {"primary", "junior", "senior", "cet4", "cet6", "adv", "grad"};

/* 墙、墙线、桌身、桌面；和 tools/pack_assets.py 的 WALL 一致 */
const nw_look_t NW_LOOK[NW_LOOKS] = {
    {0xF6D7A7, 0xE3B57C, 0xB07845, 0xC88A52}, {0xBFE3E0, 0x8CC7C1, 0x9C6B48, 0xB47C55},
    {0xC9D3EE, 0x9DAAD6, 0x7B5236, 0x916243}, {0xD9D4C7, 0xB5AD99, 0x5E4B3C, 0x715A48},
    {0xE8DDF0, 0xC3B0D6, 0x6A4A3A, 0x7E5A47}, {0x24324F, 0x33466E, 0x4A3A30, 0x5B483B},
    {0x2A2440, 0x3E3560, 0x4A3A30, 0x5B483B},
};

/* 道具锚点和原型 draw.js 的 PROP_DRAW 一致；笔记本电脑从桌面正中挪到桌子前沿（不挡 NABO 胸前），这时铅笔盒左移（见 nw_scene_home.c） */
const nw_prop_t NW_PROP[NW_PROPS] = {
    {"pencilcase", "铅笔盒", 0, 87, 197, 0}, {"clock", "闹钟", 50, 34, 52, 1},
    {"lamp", "台灯", 150, 28, 176, 0},       {"plant", "绿植", 300, 229, 176, 0},
    {"globe", "地球仪", 500, 59, 176, 0},    {"shelf", "书架", 800, 203, 104, 1},
    {"cert", "奖状", 1100, 35, 91, 1},       {"phones", "耳机", 1600, 160, 204, 0},
    {"laptop", "笔记本电脑", 2500, 120, 214, 0}, {"mug", "咖啡杯", 3500, 157, 177, 0},
    {"map", "世界地图", 4500, 201, 40, 1},   {"trophy", "奖杯", 6000, 229, 176, 0},
};

const char *const NW_TOY_NAME[NW_TOYS] = {
    "小黄鸭", "小火箭", "仙人掌", "雪花球", "魔方", "小恐龙", "纸飞机", "沙漏", "贝壳", "风车",
    "熊猫公仔", "不倒翁", "樱花盆栽", "小灯笼", "椰子", "大象木雕", "竹蜻蜓", "折扇", "鲤鱼旗", "骆驼",
    "埃菲尔铁塔", "大本钟", "郁金香", "小提琴", "足球", "城堡", "红色电话亭", "套娃", "比萨斜塔", "天鹅",
    "自由女神", "棒球", "冲浪板", "校车", "枫叶", "玛雅面具", "羊驼", "牛仔帽", "汉堡", "火烈鸟",
    "考拉", "袋鼠", "回旋镖", "金字塔", "长颈鹿", "非洲鼓", "小狮子", "珊瑚", "歌剧院", "猴面包树",
    "企鹅", "北极熊", "极光瓶", "冰屋", "帆船", "热气球", "望远镜", "宇航员", "化石", "指南针",
};
const char *const NW_TOY_SLUG[NW_TOYS] = {
    "duck", "rocket", "cactus", "snowglobe", "cube", "dino", "plane", "hourglass", "shell", "windmill",
    "panda", "tumbler", "sakura", "lantern", "coconut", "elephant", "dragonfly", "fan", "koinobori", "camel",
    "eiffel", "bigben", "tulip", "violin", "football", "castle", "phonebox", "matryoshka", "pisa", "swan",
    "liberty", "baseball", "surfboard", "schoolbus", "maple", "mayamask", "alpaca", "cowboyhat", "burger", "flamingo",
    "koala", "kangaroo", "boomerang", "pyramid", "giraffe", "drum", "lion", "coral", "operahouse", "baobab",
    "penguin", "polarbear", "aurorajar", "igloo", "sailboat", "balloon", "telescope", "astronaut", "fossil", "compass",
};

const char *const NW_ACC_ID[NW_ACCS] = {"flower", "bow", "scarf", "star", "shades", "cape", "phones", "gradcap"};
const char *const NW_ACC_NAME[NW_ACCS] = {"小红花", "蝴蝶结", "小围巾", "星星发卡", "墨镜", "小披风", "小耳机", "学士帽"};
const char *const NW_SPRITE_KEY[NW_STAGES] = {"doudou", "paopao", "xingxing", "huohuo", "langlang", "jiguang"};
const char *const NW_SPRITE_NAME[NW_STAGES] = {"豆豆", "泡泡", "星星", "火火", "浪浪", "极光"};
const char *const NW_RANK_NAME[8] = {"童生", "秀才", "举人", "贡士", "进士", "探花", "榜眼", "状元"};
const char *const NW_ACH_NAME[7] = {"第一个 100 词", "词汇量 1000", "连续 7 天", "连续 30 天",
                                    "第一枚护照章", "一次通过结业考", "对战 10 连胜"};

int nw_unit_stage(int unit)
{
    int w = unit * NW_UNIT;
    for (int s = 0; s < NW_STAGES; s++) {
        if (w < nw_stage_end[s]) return s;
    }
    return NW_STAGES - 1;
}

static const char *s_home;

void nw_set_home(const char *city) { s_home = city; }

const char *nw_unit_city(int unit)
{
    int st = nw_unit_stage(unit), k = unit - nw_stage_start[st] / NW_UNIT;
    /* 旅行从家乡出发：小学第 1 站是家乡，后面按原来的顺序，跳过和家乡重名的 */
    if (st == 0 && s_home) {
        if (k == 0) return s_home;
        for (int i = 0; i < N(C0); i++) {
            if (!strcmp(C0[i], s_home)) continue;
            if (--k == 0) return C0[i];
        }
        return "";
    }
    const nw_stage_info_t *si = &NW_STAGE_INFO[st];
    return k < si->n_cities ? si->cities[k] : "";
}
