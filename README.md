# malloc_challenge (week7)

## 課題内容
課題：元のmallocプログラム(simple_malloc.cと同じ)のメモリ使用効率と処理速度を向上させる  
1. Best-fit mallocへの改造
   simple_malloc.cでは、十分な大きさの容量のうち、一番最初に見つけたものを選んでいる。  
   これを、十分な大きさの空き容量のうち最も小さいものを選ぶように変更した。  
   今回はFreelistをサイズごとに分けているため、十分な大きさ以下のlistは探索しない。  
   十分な大きさを持っているリストを、サイズが小さいものが入っているリストから順番に探索していく。  
   リストを探索する中で、best_metadataを保持しておき、より良いものがあったらそれに更新する。  
   良いものが見つかれば、次のリストは探索しない。    

2. Freelist binの実装
   Freelistをsizeごとに14個に分け、それをリストで管理している。  
   容量が小さい空き容量の方が多いと考え、以下のように振り分けた。  
   size < 1000 の空き容量は、size / 100 のindexへ  
   1000 <= size < 1500 の空き容量は、index10へ  
   1500 <= size < 2000 の空き容量は、index11へ  
   2000 <= size < 2500 の空き容量は、index12へ  
   2500 <= size の空き容量は、index13へ　　  

3. 空になったページの返却  
   pageは4096byte(4KB)の倍数ごとに行う必要がある。pageを管理する必要があったため、metadata + object の構成に、pageを加えた。    
   各ページの先頭にpageを配置し、それを連結リストで繋ぐようにしている。    
   例) page metadata object ..... metadata object | page metadata .....  
   なお、ページを跨ぐようなmetadataおよびobjectはないようにしている。  
   1つの容量を空にする際に、そのmetadataが持っているページを見て、ページの持つallocated_countを確認する。  
   allocated_countが0になるならば、ページに使用中の場所がないということになるので、OSにそのページを返す。    

4. 空き領域の右結合の実装   
   1つの容量を空にする際に、それによってそれが属するpageのallocated_countが0にならないならば、右結合の必要があるかを確認する。    
   空になったpageの右を調べ、右の要素が存在してかつ空ならば、それを結合する。    

## 発展内容
左結合の実装を行なった。右結合とあまり変わらないが、空になったpageの左を調べ、左の要素が存在してかつ空ならば、それを結合する。    
この時、左と右の両方に空容量が存在する場合は、3つの容量をまとめて結合する。    
なお、授業スライドで述べられていた、空き容量が連続する連鎖についてだが、容量が開くたびに、左と右に空き容量がないかを確認しているので、「元々空・元々空・今回空になった」などの状況は発生しないと判断した。    

## 実行結果  
変更前のsimple_malloc.c
| Challenge | #1 | #2 | #3 | #4 | #5 | 
|------|------|------|------|------|------|
| Time [ms] | 11 | 4 | 76 | 10022 | 7503 |
| Utilization [%] | 70 | 40 | 9 | 15 | 15 |


自分のmy_malloc.c
| Challenge | #1 | #2 | #3 | #4 | #5 | 
|------|------|------|------|------|------|
| Time [ms] | 506 | 454 | 237 | 111 | 127 |
| Utilization [%] | 65 | 31 | 43 | 84 | 83 |

## 感想
#1〜#3のTimeは、simple_malloc.cより悪くなってしまった。Challengeの複雑さに関係なく、TimeとUtilizationが良くなるようなプログラムを考えるべきだったと思う。  
また、用意されていたvoid my_finalize()の中身の実装ができていない。Challengeの最後に呼ばれるとあったので、最後まで使用中だったpageを全てOSに返却するコードを書いた。しかし、それを実行するとUtilizationの計算がおかしくなり、2147483647のような壊れた数値が表示されてしまったので、コメントアウトしている。

# 以下は元からREADMEに記載されていたもの

[![Open in Cloud Shell](https://gstatic.com/cloudssh/images/open-btn.svg)](https://shell.cloud.google.com/cloudshell/editor?cloudshell_git_repo=https%3A%2F%2Fgithub.com%2Fhikalium%2Fmalloc_challenge&cloudshell_open_in_editor=malloc.c&cloudshell_workspace=malloc)

- `malloc` is the malloc challenge. Please read this doc and [malloc/malloc.c](./malloc/malloc.c) for more information.
- `visualizer/` contains a visualizer of malloc traces.

## Instruction

Your task is implement a better malloc logic in [malloc.c](./malloc/malloc.c) to improve the speed and memory usage.

## How to build & run a benchmark

```
# clone this repo
git clone https://github.com/hikalium/malloc_challenge.git

# move into malloc dir
cd malloc_challenge
cd malloc

# build
make

# run a benchmark (for score board)
make run

# run a small benchmark for tracing (NOT for score board, just for visualization and debugging purpose)
make run_trace
```

If the commands above don't work, please make sure the following packages are installed:
```
# For Debian-based OS
sudo apt install make clang
```

Alternatively, you can build and run the challenge directly by running:

```
gcc -Wall -O3 -lm -o malloc_challenge.bin main.c malloc.c simple_malloc.c
./malloc_challenge.bin
```

## Acknowledgement

This work is based on [xharaken's malloc_challenge.c](https://github.com/xharaken/step2/blob/master/malloc_challenge.c). Thank you haraken-san!

