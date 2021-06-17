# Homework Week7

## 実行方法

real_mallocディレクトリ内で下記のコマンドを実行してください。

```
make run
```

## 結果
出力結果は次のようになりました。

```
(base) MacBook-Air:real_malloc R$ time make run
cc -o malloc_challenge.bin main.c malloc.c simple_malloc.c -O3 -Wall -g -lm
./malloc_challenge.bin
Challenge 1: simple malloc => my_malloc_best_fit
Time: 11 ms => 2768 ms
Utilization: 70% => 70%
==================================
Challenge 2: simple malloc => my_malloc_best_fit
Time: 8 ms => 1182 ms
Utilization: 40% => 40%
==================================
Challenge 3: simple malloc => my_malloc_best_fit
Time: 283 ms => 1198 ms
Utilization: 8% => 50%
==================================
Challenge 4: simple malloc => my_malloc_best_fit
Time: 42071 ms => 19956 ms
Utilization: 15% => 71%
==================================
Challenge 5: simple malloc => my_malloc_best_fit
Time: 34784 ms => 12627 ms
Utilization: 15% => 74%
==================================


Challenge 1: simple malloc => my_malloc_worst_fit
Time: 10 ms => 3712 ms
Utilization: 70% => 69%
==================================
Challenge 2: simple malloc => my_malloc_worst_fit
Time: 10 ms => 1085 ms
Utilization: 40% => 39%
==================================
Challenge 3: simple malloc => my_malloc_worst_fit
Time: 236 ms => 264931 ms
Utilization: 7% => 4%
==================================
Challenge 4: simple malloc => my_malloc_worst_fit
Time: 38392 ms => 911574 ms
Utilization: 15% => 7%
==================================
Challenge 5: simple malloc => my_malloc_worst_fit
Time: 28210 ms => 1616906 ms
Utilization: 15% => 7%
==================================

real	49m41.936s
user	30m55.046s
sys	0m12.520s
```


## 概要

### Best-fit / Worst-fit
- 概要
    - simple_malloc.cのsimple_malloc関数を改変して、malloc.c 内のmy_malloc_best_fit関数(Best-fit), my_malloc_worst_fit関数(Worst-fit)として記載しました。その他には、best-fitとWorst-fitの結果を連続で出力するためにmain.cを少し書き換えました。
    - simple mallocがFirst-fitの結果として出力されています。改変した結果のBest-fit, Worst-fitと比較を行っています。
- Best-fit
    - Best-fitでは、Challenge 1, 2以外の、3, 4, 5では大幅にUtilizationが改善されました。実行時間は改善したものと悪化したものと両方観測されました。
    - First-fitははじめに見つけた格納可能な領域に納めるため、領域検索にかかる最悪計算量はO(n)ですが、平均時間計算量はO(n/2)であるのに対して、Best-fitは全ての格納可能な領域のうち一番小さいものを探しているため必ず時間計算量がO(n)です。したがって、効率の良い格納がされていない場合には、時間計算量はBest-fitの方が多くなると考えられます。
- Worst-fit
    - Best-fitで改善しなかったケースについてはUtilizationがあまり変わらず(-1%)、Best-fitで大幅に改善したケースについては計算時間が大幅に大きくなりUtilizationが低下しました。ケースの性質が反映されていると感じました。
    - フラグメンテーションが進んでしまったためにO(n)のnが大きくなってしまい、大幅に計算時間がかかってしまったのではないかと推測します。

- その他
    - どのようにテストを書けば良いかわかりませんでした。


### 空き領域の結合の実装方針
実装方針について考えてみました。  

- 双方向リストにしない場合
    - 領域をポインタで初めから辿っていき、今いる位置を指すポインタ(metadata)と一つ前を指すポインタprevとサイズprev->nextを用いてアドレス値の比較を行い、一つ前を指すポインタprevからサイズprev->next分進んだ位置とmetadataの位置が一致した場合に、ノードを繋ぎかえる。また、空き領域が連続している可能性があるため、prev->next->nextについても同様に比較を行い、空き領域が続いている場合ノードを繋ぎかえる。

- 双方向リストにする場合
    - 全体の構造体を双方向リストに変更することによって、ノードを前から辿ってprevを保存していなくてもノードに対して操作を加えることができる。free関数内で、与えられたポインタの前後を見ることで、3通りのパターンに従ってノードを繋ぎかえる。
