#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <time.h>
#define INF 1e9
#define AC 1
#define ER 0
#define COMMON_LEN 100
#define MAX_LEN 100
#define MAX_AREA 28
#define MAX_DAY 31
#define MAX_WATER 401
#define MAX_FOOD 601
#define END_POINT 27
#define VILIAGE 15
#define MINE 12
#define NO -1

typedef char* String;

// 天气枚举，晴朗：SUNNY，高温：SCORCH，沙暴：SANDSTORM
enum weather { SUNNY = 1, SCORCH, SANDSTORM };

struct poi {
    int point; // 点
    int depth; // 深度
} queue[100];

struct ans {
    int value; // 答案值
    int record; // 答案状态
};

// 细节记录，将答案细节登记好
struct detail {
    int day;
    int point;
    int money;
    int water;
    int food;
    int buy_water;
    int buy_food;
    int buy_cost;
    int cost_water;
    int cost_food;
    int is_mine;
    int is_stay;
    int is_walk;
} detail_record[MAX_DAY];

// 存储文件内容，读完内容后关闭文件指针再执行程序
char file_data[MAX_LEN][MAX_LEN]; 

// 时间计数变量
clock_t time1;
float t_dif;
int time_limit = 8000; // 限制求解时间为 8000 秒

// 每天结束游戏后统计各地区的最大受益
int max_profit[MAX_AREA];

// 广度优先基础数组，记录到达某个地点最少需要多少天
int bfs_record[MAX_AREA];
int queue_len;

// 答案输出
struct ans global_ans; // 全局记录答案
int output_ans[MAX_DAY][4];

// ============================ 初始化全局变量 =============================
int adj_list[MAX_AREA][MAX_AREA]; // 简易邻接表，每行第一个元素记录表长度
int D[MAX_AREA][MAX_AREA]; // 邻接矩阵
int profit[MAX_DAY][MAX_AREA][MAX_WATER][MAX_FOOD]; // (day, point, water, food) 剩余资金
int path[MAX_DAY][MAX_AREA][MAX_WATER][MAX_FOOD]; // 路径记录变量
int t[MAX_DAY] = { 0, 2, 2, 1, 3, 1, 2, 3, 1, 2, 2, 3, 2, 1, 2, 2, 2, 3, 3,
 2, 2, 1, 1, 2, 1, 3, 2, 1, 1, 2, 2 }; // 天气
int base_cost_water[MAX_DAY] = { 0, 8, 8, 5, 10, 5, 8, 10, 5, 8, 8, 10, 8,
 5, 8, 8, 8, 10, 10, 8, 8, 5, 5, 8, 5, 10, 8, 5, 5, 8, 8 }; // 水基础消耗量
int base_cost_food[MAX_DAY] = { 0, 6, 6, 7, 10, 7, 6, 10, 7, 6, 6, 10, 6, 
 7, 6, 6, 6, 10, 10, 6, 6, 7, 7, 6, 7, 10, 6, 7, 7, 6, 6 }; // 食物基础消耗量
int M = 10000; // 初始资金数
int mw = 3; // 每箱水的质量
int mf = 2; // 每箱食物的质量
int cw = 5; // 每箱水的基准价格
int cf = 10; // 每箱食物的基准价格
int lm = 1200; // 玩家负重上限
int ea = 1000; // 挖矿一次基础收益
// ============================       END      ============================

// 拼接路径，将结果存在 des 中
int resolvePath(String des, int des_len, String dirname, String path);

// 逐行读取 CSV 文件数据，存入内存中
int readCSVByLine(FILE* file, int ignore);

// 将文件内容转为可读矩阵
int takeFile2AdjacentMatrix(int line_num);

// 构造简易邻接表，提升算法效率，以空间换时间
void matrix2List();

// 计算水和食物的重量
int weight(int water, int food);

// 计算购买价格
int price(int water, int food, int point);

// 获取当前运行时间
float now();

// 广度优先搜索
void bfs(int begin, int depth);

// （day, point, water, food）表示在 day 天，地点 point 停留时，剩余 water 箱水和 food 箱食物时的最大剩余资金
void dp();

// 将状态进行记录，利用位运算将四种数据压缩成一个数据
// 如果采用结构体存储，则需要 4 倍的空间
struct status {
    int day;
    int point;
    int water;
    int food;
};
int compress(int day, int point, int water, int food);

// 解压数据，将数据还原到结构体
struct status decompress(int);

// 根据记录生成答案表格
void produceTable(int data);

// 验证答案是否有效
bool verifyAns();

// 日志文件
FILE *_log;

int testResolvePath(); // 测试函数
void loggin(const char* funcName, const char* msg); // 日志函数
void print_D(); // 打印邻接矩阵
void print_list(); // 打印邻接表
struct ans max(int n, ...); // 返回 n 个答案的最大值 

int main() {

    _log = fopen("record.txt", "w");
    
    // ========================    读取文件内容     ========================
    // loggin("func: resolvePath", testResolvePath() == AC ? "Accepted" : "Error");

    char data_dir[COMMON_LEN] = "./data";
    char data_file_path[COMMON_LEN] = "/map-first.csv";
    char map_data_path[COMMON_LEN];
    resolvePath(map_data_path, COMMON_LEN, data_dir, data_file_path);

    loggin("map csv path", map_data_path);

    FILE* map_csv = fopen(map_data_path, "r");
    int line_len = readCSVByLine(map_csv, 2);
    
    if (fclose(map_csv) != 0)
        printf("文件 %s 关闭出错！", map_data_path);
    // ========================        END         ========================


    // ==================== 将地图信息转换为邻接矩阵数据 ====================
    takeFile2AdjacentMatrix(line_len);
    matrix2List();
    // print_list();
    // ====================            END            ====================

    // =========================     验证答案     =========================
    // data 目录下要有 ans-first.csv文件，启用函数读取文件内容进行验证
    // 求解的话将这个区域注释掉
    // fclose(_log);
    // freopen("ans.txt", "w", stdout);
    // if (verifyAns()) {
    //     printf("答案有效，恭喜 AC！\n");
    // }
    // return 0;
    // ====================            END            ====================


    // =========================  初始化表格状态  =========================

    // 填充初始值
    memset(profit, NO, sizeof profit);
    memset(bfs_record, NO, sizeof bfs_record);
    memset(path, NO, sizeof path);
    bfs(1, 0);

    // 计算水和食物在不同购买数量组合下的剩余资金
	for (int wbuy = 0; wbuy < MAX_WATER; wbuy++) {
		for (int fbuy = 0; fbuy < MAX_FOOD; fbuy++) {
			// 初始资金 - 购买水量 * 水基础价格 - 购买食物量 * 食物基础价格
            if (weight(wbuy, fbuy) <= lm && price(wbuy, fbuy, 1) <= M) {
                profit[0][1][wbuy][fbuy] = M - price(wbuy, fbuy, 1);
            }
		}
	}
    // =========================       END       =========================

    // 动态规划表格法
    // =========================     开始打表     =========================
    time1 = clock();
    global_ans.value = -INF;
    global_ans.record = NO;
    dp();
    // =========================       END       =========================

    printf("30 天游戏最优解为：%d\n", profit[30][END_POINT][0][0]);
    fprintf(_log, "30 天游戏最优解为：%d\n", profit[30][END_POINT][0][0]);

    // =========================     打印答案     =========================
    produceTable(global_ans.record);
    struct status a = decompress(global_ans.record);
    output_ans[a.day + 1][0] = a.day + 1;
    output_ans[a.day + 1][1] = END_POINT;
    output_ans[a.day + 1][2] = 0;
    output_ans[a.day + 1][3] = 0;
    puts("\n======================================\n");
    fputs("\n======================================\n", _log);
    for (int i = 0; i <= a.day + 1; i++) {
        for (int j = 0; j < 4; j++) {
            printf("%d%s", output_ans[i][j], j == 3 ? "\n" : ",\t");
            fprintf(_log, "%d%s", output_ans[i][j], j == 3 ? "\n" : ",\t");
        }
    }
    puts("\n======================================\n");
    fputs("\n======================================\n", _log);
    // =========================       END       =========================

    fclose(_log);

    return 0;
}

int resolvePath(String des, int des_len, String dirname, String path) {
    int dir_len = strlen(dirname);
    int path_len = strlen(path);
    if (dir_len + path_len >= des_len) {
        return ER;
    }
    strcpy(des, dirname);
    strcat(des, path);
    return AC;
}

int readCSVByLine(FILE* file, int ignore) {
    if (file == NULL) return ER;
    char line[MAX_LEN];
    int line_index = 0;
    while (fgets(line, MAX_LEN - 1, file) != NULL) {
        if (line_index >= ignore)
            strcpy(file_data[line_index - ignore], line);
        line_index++;
    }
    return line_index - ignore;
}

int takeFile2AdjacentMatrix(int line_num) {
    char* dis;
    char* line;
    int j = 0;
    for (int i = 0; i < line_num; i++, j = 0) {
        line = file_data[i];
        dis = strtok(line, ",\n");
        while (dis) {
            if (j != 0)
                D[i + 1][j] = D[j][i + 1] = atoi(dis);
            dis = strtok(NULL, ",\n");
            j++;
        }
    }
    return AC;
}

int takeFile2Matrix(int line_num) {
    char* dis;
    char* line;
    for (int i = 0, j = 0; i < line_num; i++, j = 0) {
        line = file_data[i];
        dis = strtok(line, ",\n");
        while (dis) {
            output_ans[i][j] = atoi(dis);
            dis = strtok(NULL, ",\n");
            j++;
        }
    }
    return AC;
}

void matrix2List() {
    for (int i = 1; i < MAX_AREA; i++)
        for (int j = i + 1; j < MAX_AREA; j++)
            if (D[i][j]) {
                adj_list[i][++adj_list[i][0]] = j;
                adj_list[j][++adj_list[j][0]] = i;
            }
                
}

void dp() {

    int day, point, _food, _water;
    int fb, wb;
    int adj_len, k;
    int _cost_w, _cost_f;
    int water_spa, food_spa;
    struct ans walk_profit, stay_profit, stay_mine_profit, walk_buy_profit, stay_buy_profit;
    struct ans mid, ans;
    int w1, f1;
	int yes_money, today_money_cost;
    float tt;
    int next = 1;
    int n;

    for (day = 1; day < MAX_DAY; ++day) {
        // 每天根据前一天的最优解更新当天的状态

        for (point = 1; point < MAX_AREA; ++point) {

            // 如果到达当前点的天数少于最少天数，说明不正常
            if (day < bfs_record[point]) continue;

            // 遍历每个点
            adj_len = adj_list[point][0]; // point 邻接点数量

            if (point == END_POINT) {
                // 如果今天在终点结束活动
                // 那么终点的最优解只可能是昨天就到达终点，以及昨天在终点的邻接点走过来
                stay_profit.value = -INF;
                stay_profit.record = NO;
                if (profit[day - 1][point][0][0] >= 0) {
                    // 昨天就到达终点，游戏已经结束，没有消耗
                    stay_profit.value = profit[day - 1][point][0][0];
                    stay_profit.record = compress(day - 1, point, 0, 0);
                }
                
                walk_profit.value = -INF;
                walk_profit.record = NO;
                if (t[day] != SANDSTORM) {
                    // 今天不是沙暴才能走
                    // 行走两倍消耗
                    _cost_w = 2 * base_cost_water[day];
                    _cost_f = 2 * base_cost_food[day];

                    for (int i = 1; i <= adj_len; ++i) {
                        k = adj_list[point][i]; // 当前邻接点
                        
                        // 由于今天到达终点，只要昨天能满足今天行走的资源消耗，
                        // 则其他食物组合都不是最优解
                        
                        if (profit[day - 1][k][_cost_w][_cost_f] >= 0) {
                            mid.value = profit[day - 1][k][_cost_w][_cost_f];
                            mid.record = compress(day - 1, k, _cost_w, _cost_f);
                            walk_profit = max(2, walk_profit, mid);
                        }

                        t_dif = now();
                        if (t_dif > time_limit) {
                            goto time_out; // 求解超时
                        }
                    }
                }

                t_dif = now();
                ans = max(2, stay_profit, walk_profit);
                if (global_ans.value < ans.value) {
                    global_ans = ans;
                }
                profit[day][point][0][0] = ans.value;
                path[day][point][0][0] = ans.record; // 记录前驱

                if (ans.value > max_profit[point])
                    max_profit[point] = ans.value;

                if (ans.value >= 0) {
                    printf("经过 %d 秒...\n", (int)t_dif);
                    printf("当前答案: (%d, %d, %d, %d) = %d\n", day, point, 0, 0, ans.value >= 0 ? ans.value : NO);
                    puts("\n======================================\n");

                    fprintf(_log, "经过 %d 秒...\n", (int)t_dif);
                    fprintf(_log, "当前答案: (%d, %d, %d, %d) = %d\n", day, point, 0, 0, ans.value >= 0 ? ans.value : NO);
                    fputs("\n======================================\n", _log);
                }
                
            } else {
                // 今天到达的点不是终点
   
                // 未到达终点时，玩家物资水和食物都不能为 0
                for (_water = 1; _water < MAX_WATER; ++_water) {
                    if (weight(_water, 0) > lm) break;
                    for (_food = 1; _food < MAX_FOOD; ++_food) {
                        // 遍历今天会遇到的各种物资组合

                        // 超重情况
                        if (weight(_water, _food) > lm) break;
                        
                        // 情况一，由昨天什么都不做，直接停留而来
                        
                        stay_profit.value = -INF;
                        stay_profit.record = NO;
                        _cost_w = base_cost_water[day];
                        _cost_f = base_cost_food[day];
						w1 = _water + _cost_w;
						f1 = _food + _cost_f;
                        if (w1 < MAX_WATER && f1 < MAX_FOOD) {
                            if (profit[day - 1][point][w1][f1] >= 0) {
                                stay_profit.value = profit[day - 1][point][w1][f1];
                                stay_profit.record = compress(day - 1, point, w1, f1);
                            }
                        }

                        stay_buy_profit.value = -INF;
                        stay_buy_profit.record = NO;
                        if (point == VILIAGE) {
							// 停留时，今天在村庄，可以购买物资
							for (wb = 0; wb < _water; ++wb) {
								for (fb = 0; fb <= _food; ++fb) {
									// 昨天剩余物资
									water_spa = _cost_w + (_water - wb);
									food_spa = _cost_f + (_food - fb);
                                    if (water_spa >= MAX_WATER || food_spa >= MAX_FOOD) break;
									yes_money = profit[day - 1][point][water_spa][food_spa];
									today_money_cost = price(wb, fb, point);
									if ((yes_money >= 0) && (today_money_cost <= yes_money)) {
										// 购买组合可以
                                        mid.value = yes_money - today_money_cost;
                                        mid.record = compress(
                                            day - 1, point, water_spa, food_spa
                                        );
										stay_buy_profit = max(2, stay_buy_profit, mid);
									}

                                    t_dif = now();
                                    if (t_dif > time_limit) {
                                        goto time_out; // 求解超时
                                    }
								}
							}
                        }

                        stay_mine_profit.value = -INF;
                        stay_mine_profit.record = NO;
						if (point == MINE) {
							// 停留时，今天在矿山，不挖矿就是直接停留的收益
							// 挖矿 3 倍消耗
							_cost_w = 3 * base_cost_water[day];
							_cost_f = 3 * base_cost_food[day];
                            w1 = _water + _cost_w;
                            f1 = _food + _cost_f;
                            if (w1 < MAX_WATER && f1 < MAX_FOOD) {
                                if (profit[day - 1][point][w1][f1] >= 0) {
                                    // 昨天可以到达矿山，今天挖矿获取收益
                                    stay_mine_profit.value = profit[day - 1][point][w1][f1] + ea;
                                    stay_mine_profit.record = compress(
                                        day - 1, point, w1, f1
                                    );
                                }
                            }
						}
                        
                        // 情况二，由昨天从相邻点走到当前点
                        walk_profit.value = -INF;
                        walk_profit.record = NO;
                        walk_buy_profit.value = -INF;
                        walk_buy_profit.record = NO;
                        if (t[day] != SANDSTORM) {
                            // 不为沙暴
                            // 行走两倍消耗
                            _cost_w = 2 * base_cost_water[day];
                            _cost_f = 2 * base_cost_food[day];
                            w1 = _water + _cost_w;
                            f1 = _food + _cost_f;
                            if (w1 < MAX_WATER && f1 < MAX_FOOD) {
                                for (int i = 1; i <= adj_len; ++i) {
                                    k = adj_list[point][i]; // 当前邻接点
                                    if (profit[day - 1][k][w1][f1] >= 0) {
                                        mid.value = profit[day - 1][k][w1][f1];
                                        mid.record = compress(day - 1, k, w1, f1);
                                        walk_profit = max(2, walk_profit, mid);
                                    }

                                    if (point == VILIAGE) {
                                        // 今天结束行走后在村庄停留，可以购买物资
                                        for (wb = 0; wb < _water; ++wb) {
                                            for (fb = 0; fb <= _food; ++fb) {
                                                // 昨天剩余物资
                                                water_spa = _cost_w + (_water - wb);
                                                food_spa = _cost_f + (_food - fb);
                                                if (water_spa >= MAX_WATER || food_spa >= MAX_FOOD) break;
                                                // 现在昨天的钱是从邻接点 K 获取，别搞错了
                                                yes_money = profit[day - 1][k][water_spa][food_spa];
                                                today_money_cost = price(wb, fb, point);
                                                // 昨天状态可达以及今天消费允许时，
                                                if ((yes_money >= 0) && (today_money_cost <= yes_money)) {
                                                    // 购买组合可以
                                                    mid.value = yes_money - today_money_cost;
                                                    mid.record = compress(
                                                        day - 1, k, water_spa, food_spa
                                                    );
                                                    walk_buy_profit = max(2, walk_buy_profit, mid);
                                                }
                                            }

                                            t_dif = now();
                                            if (t_dif > time_limit) {
                                                goto time_out; // 求解超时
                                            }
                                        }
                                    }
                                }
                            }
                            
                        }

                        ans = max(5, 
                        stay_profit, stay_buy_profit, stay_mine_profit,
                        walk_profit, walk_buy_profit);
                        profit[day][point][_water][_food] = ans.value;
                        path[day][point][_water][_food] = ans.record;
                        if (ans.value > max_profit[point])
                            max_profit[point] = ans.value;

                        t_dif = now();
                        if (t_dif > time_limit) {
                            goto time_out; // 求解超时
                        }
                    }
                }
            }
        }
        // 一天求解结束，打印各个地区的最大值
        printf("第 %d 天：\t\t\t用时 %d 秒\n", day, (int)now());
        fprintf(_log, "第 %d 天：\t\t\t用时 %d 秒\n", day, (int)now());
        for (point = 1; point < MAX_AREA; point++) {
            printf("(%2d, %4d)%s", point, max_profit[point], 
            ((point) % 3 == 0 || point == MAX_AREA - 1) ? "\n" : ",");
            fprintf(_log, "(%2d, %4d)%s", point, max_profit[point], 
            ((point) % 3 == 0 || point == MAX_AREA - 1) ? "\n" : ",");
        }
        puts("\n======================================\n");
        fputs("\n======================================\n", _log);
    }
    return ;

    time_out:
    n = (int)(now());
    printf("求解超时，强制结束!    用时: %d 秒\n", n);
    fprintf(_log, "求解超时，强制结束! 用时: %d\n", n);
}

int weight(int water, int food) {
    return water * mw + food * mf;
}

int price(int water, int food, int point) {
    int a = (water * cw + food * cf);
    return point == VILIAGE ? (2 * a) : a;
}

float now() {
    float t2 = (clock() - time1) * 1.0 / CLOCKS_PER_SEC;
    return t2;
}

void bfs(int begin, int depth) {
    struct poi p = { begin, depth };
    struct poi head;
    bfs_record[begin] = depth;
    int quene_head = 0;
    int adj_len;
    queue[queue_len++] = p; // 初始节点入队
    while (quene_head != queue_len) {
        head = queue[quene_head];
        adj_len = adj_list[head.point][0];
        for (int i = 1; i <= adj_len; i++) {
            p.point = adj_list[head.point][i];
            if (bfs_record[p.point] == NO) {
                p.depth = head.depth + 1;
                bfs_record[p.point] = p.depth;
                queue[queue_len++] = p; // 新结点入队
            }
        }
        quene_head++; // 头结点出队
    }
}

int compress(int day, int point, int water, int food) {
    // 压缩原理，一个 int 在现代的大多数机器上是 32 位
    // 而 day 和 point 在本关都不超过 31
    // 31 刚好可以用五位二进制 1 表示，也就是 10 位二进制即可记录两个数据
    // water 最大 400，food 最大 600 
    // 400 不超过 511，可以用 9 位二进制表示，600 就需要 10 位二进制了
    // 那么我们约定，32 位数据的位置分配如下，water 和 food 统一 10 位，方便运算
    // 高次幂位 -> 低次幂
    // 00 11111(day) 11111(point) 1111111111(water) 1111111111(food)
    // 这样就可以通过位运算将 4 个数压缩成一个数了
    
    int data = 0;
    data |= day << 25;   // day 占用 5 位
    data |= point << 20; // point 占用 5 位
    data |= water << 10; // water 占用 10 位
    data |= food;        // food 占用 10 位
    return data;
}

// 解压数据，将数据还原到结构体
struct status decompress(int data) {
    // 依次解压数据
    struct status res;
    res.food = 1023 & data; data >>= 10; // 右移 10 位，去掉 food
    res.water = 1023 & data; data >>= 10; // 右移 10 位 去掉 water
    res.point = 31 & data; data >>= 5; // 右移 5 位 去掉 point
    res.day = 31 & data;
    return res;
}

int testResolvePath() {
    char a[20] = "Hello";
    char b[20] = " world";
    char c[20] = "Hello world";
    if (resolvePath(a, 20, a, b) == ER) return ER;
    if (strcmp(a, c) != 0) return ER;
    return AC;
}

void loggin(const char* funcName, const char* msg) {
    printf("%s: %s\n", funcName, (void*)msg == NULL ? "" : msg);
}

void print_D() {
    // 打印邻接矩阵
    for (int i = 1; i < MAX_AREA; i++)
        for (int j = 1; j < MAX_AREA; j++)
            printf("%d%s", D[i][j], j == MAX_AREA - 1 ? "\n" : ", ");
}

void print_list() {
    // 打印邻接表
    int i, j, k;
    for (i = 1; i < MAX_AREA; i++) {
        printf("%d: ", i);
        k = adj_list[i][0];
        for (j = 1; j <= k; j++) {
            printf("%d%s", adj_list[i][j], j == k ? "\n" : ", ");
        }
    }
}

struct ans max(int n, ...) {
    // 可变参数表
    va_list ap;
    va_start(ap, n);

    struct ans res = { -INF, NO };
    struct ans val;
    for (int i = 0; i < n; i++) {
        val = va_arg(ap, struct ans);
        if (val.value > res.value) {
            res = val;
        }
    }
    return res;
}

void produceTable(int data) {
    // 根据全局记载的答案生成表格
    struct status ans = decompress(data);
    if (path[ans.day][ans.point][ans.water][ans.food] == NO) {
        // 没有前驱了
        output_ans[0][0] = ans.day;
        output_ans[0][1] = ans.point;
        output_ans[0][2] = ans.water;
        output_ans[0][3] = ans.food;
        return ;
    }
    produceTable(path[ans.day][ans.point][ans.water][ans.food]);
    output_ans[ans.day][0] = ans.day;
    output_ans[ans.day][1] = ans.point;
    output_ans[ans.day][2] = ans.water;
    output_ans[ans.day][3] = ans.food;
}

void detialRecord(int day, int point, int money, int water, int food,
                  int cost_water, int cost_food, int buy_water, int buy_food,
                  int buy_cost, int is_walk, int is_mine, int is_stay) {
    detail_record[day].day = day;
    detail_record[day].point = point;
    detail_record[day].money = money;
    detail_record[day].water = water;
    detail_record[day].food = food;
    detail_record[day].buy_water = buy_water;
    detail_record[day].buy_food = buy_food;
    detail_record[day].buy_cost = buy_cost;
    detail_record[day].cost_water = cost_water;
    detail_record[day].cost_food = cost_food;
    detail_record[day].is_mine = is_mine;
    detail_record[day].is_stay = is_stay;
    detail_record[day].is_walk = is_walk;
}

bool verifyAns() {
    // =========================     读取文件     =========================
    // 用上面打印出来的表格格式即可，将根据答案路线进行验证，并生成剩余资金
    char data_dir[COMMON_LEN] = "./data";
    char data_file_path[COMMON_LEN] = "/ans-first.csv";
    char ans_data_path[COMMON_LEN];
    resolvePath(ans_data_path, COMMON_LEN, data_dir, data_file_path);

    loggin("ans csv path", ans_data_path);

    FILE* ans_csv = fopen(ans_data_path, "r");
    int line_len = readCSVByLine(ans_csv, 0);
    
    if (fclose(ans_csv) != 0)
        printf("文件 %s 关闭出错！", ans_data_path);
    
    // for (int i = 0; i < line_len; i++)
    //     puts(file_data[i]);
    // =========================       END       =========================

    // =========================     转换数据     =========================
    takeFile2Matrix(line_len);
    // for (int i = 0; i < line_len; i++)
    //     for (int j = 0; j < 4; j++) {
    //         printf("%d%s", output_ans[i][j], j == 3 ? "\n" : "\t");
    //     }
    // =========================       END       =========================

    int money = M;
    int water, food;
    int last_water, last_food;
    int last_point, now_point;
    int buy_water, buy_food;
    int cost_water, cost_food;
    int buy_cost;
    puts("\n======================================\n");
    for (int i = 0; i < line_len; i++) {
        now_point = output_ans[i][1];
        water = output_ans[i][2];
        food = output_ans[i][3];

        if ((water <= 0 || food <= 0) && now_point != END_POINT) {
            printf("第 %d 天非终点物资为 0\n", i);
            return false;
        }

        if (weight(water, food) > lm) {
            printf("第 %d 天物资超重\n", i);
            return false;
        }
        
        if (i == 0) {
            // 第 0 天情况
            last_point = now_point;
            last_water = water;
            last_food = food;
            buy_cost = price(water, food, last_point);
            if (weight(water, food) > lm || buy_cost > money) {
                puts("第 0 天数据有误");
                return false;
            }
            money -= buy_cost;
            printf("%d\t%d\t%d\t%d\t%d\n", i, now_point, money, water, food);

            detialRecord(i, now_point, money, water, food, 0, 0, water, food,
            buy_cost, 0, 0, 1);
            continue;
        }
        // 正常游戏天数情况

        if (last_point != now_point) {
            // 行走情况

            if (t[i] == SANDSTORM) {
                // 沙暴却行走
                printf("第 %d 天不应该行走\n", i);
                return false;
            }

            if (D[last_point][now_point] == 0) {
                printf("第 %d 天不能从 %d 区域到达 %d 区域\n", i, last_point, now_point);
            }

            if (now_point == VILIAGE) {
                // 今天到达村庄，涉及行走消耗和物资购买
                cost_water = 2 * base_cost_water[i];
                cost_food = 2 * base_cost_food[i];

                if (last_water < cost_water || last_food < cost_food) {
                    printf("第 %d 天的剩余物资不能支撑第 %d 天的行走消耗\n", i - 1, i);
                    return false;
                }

                buy_water = cost_water + water - last_water;
                buy_food = cost_food + food - last_food;
                buy_cost = price(buy_water, buy_food, now_point);
                if (buy_cost > money) {
                    printf("第 %d 天的剩余资金不能支撑购买物资（水：%d, 食物：%d）\n", buy_water, buy_food);
                    return false;
                }
                
                money -= buy_cost;
                detialRecord(i, now_point, money, water, food, cost_water, cost_food, 
                buy_water, buy_food, buy_cost, 1, 0, 0);
            } else {
                // 非村庄的地点都是一样的行走处理
                if (water >= last_water || food >= last_food) {
                    printf("第 %d 天不应该出现物资增长\n", i);
                    return false;
                }

                cost_water = last_water - water;
                cost_food = last_food - food;
                if (cost_water != 2 * base_cost_water[i]
                && cost_food != 2 * base_cost_food[i]) {
                    printf("第 %d 天应该是行走消耗（水：%d, 食物：%d）\n", i, cost_water, cost_food);
                    return false;
                }
                detialRecord(i, now_point, money, water, food, cost_water, cost_food, 
                0, 0, 0, 1, 0, 0);
            }
        } else {
            // 停留情况
            if (now_point == VILIAGE) {
                // 今天停留在村庄，涉及停留消耗和物资购买
                cost_water = base_cost_water[i];
                cost_food = base_cost_food[i];

                if (last_water < cost_water || last_food < cost_food) {
                    printf("第 %d 天的剩余物资不能支撑第 %d 天的停留消耗\n", i - 1, i);
                    return false;
                }

                buy_water = cost_water + water - last_water;
                buy_food = cost_food + food - last_food;
                buy_cost = price(buy_water, buy_food, now_point);
                if (buy_cost > money) {
                    printf("第 %d 天的剩余资金不能支撑购买物资（水：%d, 食物：%d）\n", i, buy_water, buy_food);
                    return false;
                }
                
                money -= buy_cost;
                detialRecord(i, now_point, money, water, food, cost_water, cost_food, 
                buy_water, buy_food, buy_cost, 0, 0, 1);
            } else {
                if (water >= last_water || food >= last_food) {
                    printf("第 %d 天不应该出现物资增长\n", i);
                    return false;
                }

                cost_water = last_water - water;
                cost_food = last_food - food;
                if (cost_water == 2 * base_cost_water[i]
                && cost_food == 2 * base_cost_food[i]) {
                    printf("第 %d 天应该是挖矿或者停留消耗（水：%d, 食物：%d）\n", i, cost_water, cost_food);
                    return false;
                }

                int is_mine = 0;
                if (now_point == MINE) {
                    // 矿山
                    if (cost_water == 3 * base_cost_water[i] 
                    && cost_food == 3 * base_cost_food[i]) {
                        // 挖矿了
                        money += ea;
                        is_mine = 1;
                    }
                }

                detialRecord(i, now_point, money, water, food, cost_water, cost_food, 
                0, 0, 0, 0, is_mine, 1);
            }
        }
        
        printf("%d\t%d\t%d\t%d\t%d\n", i, now_point, money, water, food);
        last_point = now_point;
        last_water = water;
        last_food = food;
    }
    puts("\n======================================\n");

    puts("\n======================================\n");
    struct detail *p;
    char wea[20];
    for (int i = 0; i < line_len; i++) {
        p = &detail_record[i];
        printf("第 %d 天 | ", i);
        if (i != 0) {
            switch (t[i]) {
                case SUNNY:
                    strcpy(wea, "晴朗");
                    break;
                
                case SCORCH:
                    strcpy(wea, "高温");
                    break;
                
                case SANDSTORM:
                    strcpy(wea, "沙暴");
                    break;
            }
            printf("天气：%s | 水基础消耗：%d | 食物基础消耗：%d | ", 
            wea, base_cost_water[i], base_cost_food[i]);
        }
        if (p->is_walk) {
            printf("走到了 %d 区域 | 消耗 水（%d）, 食物（%d） | ", 
            p->point, p->cost_water, p->cost_food);
        }
        if (p->is_stay) {
            printf("停留在 %d 区域 | 消耗 水（%d）, 食物（%d） | ", 
            p->point, p->cost_water, p->cost_food);
        }
        if (p->buy_cost > 0) {
            printf("在 %d 区域购买了, 水（%d）, 食物（%d） | 花费了：%d | ",
            p->point, p->buy_water, p->buy_food, p->buy_cost);
        }
        if (p->is_mine) {
            printf("在 %d 区域挖矿一次，收入 %d | ", p->point, ea);
        }
        printf("剩余：资金：%d | 水：%d | 食物：%d | 背包负重：%d ",
        p->money, p->water, p->food, weight(p->water, p->food));
        putchar('\n');
    }
    return true;
}