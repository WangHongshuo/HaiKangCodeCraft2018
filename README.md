# 海康威视2018软件精英挑战赛 #
- 初赛第7名，感觉应该是没和大佬分一个组所以分数高
- 复赛具体名次不清楚，没有进决赛和复活赛
## 思路： ##
### 整体思想: ###
- 初赛：老老实实运货，路径最优，货物匹配最优，躲着自己和敌方的飞机别撞了。
- 复赛：加入价值公式进一步货物匹配最优，买新飞机最优，重写（作死）环境感知加入双步检测，老老实实运货，加入攻击部分（但是只是一架飞机）。
### 具体模块: ###
- 寻路：A*算法，但是因为时间关系没有很好的完成，只是用贪心算法保证每一步都是离目标最近。对多个高度层进行路径规划取最小路径。
- 货物抓取：对每个UAV，对所有无状态货物（无状态是指没有被其他UAV设为抓取目标，没有被拾取的）采用价值公式： `Scores = (货物价值/取货路径长度)×(货物价值/货物重量)×(货物重量/UAV载重)`
取最大的Scores对应的货物对UAV进行匹配（这里有个很有问题的地方：1、没加入电量；2、是不是应该双向匹配取Scores最大的UAV-GOODS组合，但是这样计算量会大一些）
- 移动：有优先级
- 环境感知：1、对于移动的UAV，检测下一步是否有UAV，如果是友方的，让友方先移动（递归），如果友方移动不了，选择避让，避让原则是对移动域（可移动区域10个点）的点（经过双步检测过滤）,取离目标最近的点移动，如果移动过程中出现路径交叉重叠也进行避让（会判定为撞机）；如果是敌方的，把移动域中与敌方UAV移动域重叠的部分去除，进行躲避。2、对于静止的UAV，躲避策略是远离。
- 双步检测：对将要移动到的下一个点的移动域进行环境感知。
- 买UAV：对所有无状态货物，采取价值公式： `Scores = (货物价值/停机坪离货物距离)×(货物价值/货物重量)`
取最大的Scores对应的货物，买离货物重量最近的UAV。
- 进攻：只是简单的把对方价值最大的UAV设为攻击目标，而且Attacker的数量只设置为1。
### 取货效果： ###
- 初赛大人机大地图：忘了
- 复赛人机大地图：30479
## TODO& 吐槽： ##
我感觉这次海康威视的比赛要比华为的比赛有趣的多，因为加入了人与人之间的对抗，有许多的不确定性，大家也都挺好的，交流也蛮多的，也很热心，会相互测Bug相互帮助相互鼓励。

-----------------吐槽分割线，我要开始了-----------------

1. 一个人做真的是有些累，时间和能力有限，想做的东西还有很多，就是没有时间，还要给老板浇树orz
2. 我是真的想把UAV和GOODS写成单独的类，这样在很多情况下会舒服的多。
3. A*算法没有很好的实现，如果能很好的实现了，估计运货效率上还能提高一些。
4. 真不应该把精力全放在运货上面，到复赛的时候会进攻和防守也相当重要。
5. 重写环境感知是正确的，因为初赛版本全是if人肉垒起来的，看起来太恶心了，但是重写只完成了一半，复赛就开了，而且初赛的服务器也不让用了orz
6. 大的框架还可以，就是有些小的地方扩展起来很难受，期间还为了省去局部变量初始化的耗时用了全局变量翻车了，真应该好好学设计orz

------------------吐槽分割线，我结束了------------------

总的来说，在做的过程中自己挖了好多坑，也遇到一些坑，有的能填了，有的填不了就绕过去，对于非科班的我来说，成长和收获非常多。

PS: C++就是快  )