
= もっと便利にKibanaを利用するために

ここではKibanaの新しいバージョンであるバージョン6の特徴を紹介します。

== みんなに配慮、優しい色合い

まず、大きく異なるのは全体の色味です。Kibanaのバージョン5（以降、Kibana5とします）はピンクや青など、明るい色をメインカラーとして使用していました。

ところがKibana6からは青を基調とした昆布のような色合いになっています。Kibana5と比較すると地味ですね。

なぜそんな地味カラーになってしまったのでしょう？
これにはちゃんとわけがあります。

@<code>{色盲}という言葉をみなさんご存知でしょうか？ヒトの目は網膜の中に錐体細胞という細胞を持っています。
この細胞、赤・青・緑を感じることができる物質をそれぞれ持っています。赤・青・緑の濃さを見分けて、色をいろいろ見分けることができるんですね。

色盲ではない人は3色の色を感じることができるのですが、何らかの原因で赤・青・緑の錐体細胞のどれかがうまく働かなくなってしまう人もいます。
それが、色盲という状態です。
この色盲、何と男性では20人に1人、女性の500人に1人の割合で見受けられる、という研究もあります（黄色人種の場合）。
そして、赤系の色盲になる人が1番多いのです@<fn>{kibana01-fn01}。

//footnote[kibana01-fn01][参考：https://www.nig.ac.jp/color/gen/]

Elastic社はこの色盲の方に配慮してUIの色を変更したのです。

== 楽々Dashboardセットアップ

次に特徴的なのは@<code>{APM}や@<code>{Logging}です。これは@<code>{Modules}というElastic Stackの新機能です。
「Beatsを体験する」でも触れていますが、専用の@<code>{Beats Modules}を起動すると、Elasticsearchに自動で接続・KibanaのDashboardまで作成できます。
とても便利ですね。

「じゃあもうVisualizeする必要ないのかな？」と思った方もいるかもしれません。ただこのModules、利用できるデータの種類に制限があります。
Elastic Stack6.2の時点で利用できるModulesは次のとおりです。（KibanaのUIの中で確認できます。）

 * Apacheのログ
 * Apacheのメトリクス
 * APM
 * Dockerのメトリクス
 * Kubernetesのメトリクス
 * MySQLのログ
 * MySQLのメトリクス
 * Netflow
 * Nginxのログ
 * Nginxのメトリクス
 * Redisのログ
 * Redisのメトリクス
 * システムログ
 * システムのメトリクス

@<code>{Netflow}はCisco社が開発したネットワークトラフィックの詳細情報を収集するための技術です。
@<code>{Redis}はNoSQLデータベースの1種です。

となると、このラインナップを見る限りWeb系をメインターゲットとして置いてるように見えますね。
やはりWebサービスは性能が命ですから、性能やサービス監視は簡単に構築したいものです。その需要に応えるバージョンアップといえるでしょう。

== グラフの種類も増えた！

Visualizeを利用すると、自分でグラフを作成できるというのはこれまでの章で紹介したとおりです。このVisualizeがデフォルトで利用できるグラフが増えました。

Kibana5.4から増えたグラフは次のとおりです。

 * Goal
 * Coordinate Map
 * Region Map
 * Controls
 * Vega

この中でも異彩を放つ@<code>{Vega}についてここでは取り上げたいと思います。

=== Vega

@<code>{Vega}（@<href>{https://vega.github.io/vega/}）は、The UW Interactive Data Lab（@<href>{http://idl.cs.washington.edu/about}）が作成・開発している、データを
グラフに描画するためのツールです。

Kibanaと同じなのでは？と思う方もいるかもしれませんが、Vegaはデータ・グラフを描画するための設定をJSONで管理します。一方、Kibanaはグラフ描画に利用するデータは
Elasticsearchから取得しますし、グラフの描画はGUIを用いて行います。

また、Vegaで描画できるグラフの種類（@<href>{https://vega.github.io/vega/examples/}）はKibanaよりも多いです。特にデータ分析を行う場合に利用することが多い
棒線グラフに標準偏差を記述することが可能です。

しかし、せっかくElasticsearchに投入されているデータが大量にあるのですから、それをより詳しく分析したいですよね。ということで、ベータ版ではありますがKibanaのGUIから
Vegaの機能を呼び出して利用できるようになりました。それがVisualize画面のVegaです。

//image[kibana02-vega01][Vega][scale=0.7]{
//}


注意してほしいのは、このグラフはベータ版なので開発が中止されたり無くなる可能性があります。むしろこの際Vegaを極めてもよいかもしれませんね。


== 何気に嬉しい便利機能

これから紹介する機能は筆者が「あら便利！むかしよりも進化してる！」と思ったけれど、そんなに推されていない機能です。

=== Dev Toolsの入力補完

「GoではじめるElasticsearch」の章ではコンソール上で直接ElasticsearchにQueryを発行していました。
しかし、KibanaのGUIには@<code>{Dev Tools}という画面があります。これがすばらしいのです。

//image[kibana02-devtools01][Dev Toolsの画面][scale=0.7]{
//}


なにがすばらしいのか？それは、Queryを入力する途中で入力補完が出てくるというところです。

たとえば、今Elasticsearchに存在するindexを出したいなーと思ったとします。

コンソール上でQueryを発行するのであれば、次のように手で記入しますよね。

//list[kibana02-list01][Indexの存在を確認する]{
curl -XGET localhost:9200/_cat/indices/logstash-*
//}


でも、Kibanaの@<code>{Dev Tools}で同じように記載しようとすると…？

//image[kibana02-devtools02][Queryの入力予測が出力される][scale=0.7]{
//}

@<img>{kibana02-devtools02}のように、Queryの入力予測が画面に表示されます。
毎度Queryを調べる必要がなくなりますし、コマンドラインで複雑なQueryを発行するよりも圧倒的に利便性が高いです。

Queryを発行するためには緑の@<code>{▶︎}ボタンをクリックします。

//image[kibana02-devtools03][Queryを発行した状態][scale=0.7]{
//}


JSONで値が帰ってくる場合、自動でシンタックスハイライトが適用されているので、可読性も高いです。

//image[kibana02-devtools04][jsonでデータが返却されたとき][scale=0.7]{
//}


作業用コンソールをいくつも立ち上げておくのは事故の元、と言いますが、Elasticsearchに限っていえば、@<code>{Dev Tools}を利用することで作業用ウィンドウを1つ節約できます。
みなさんも使ってみてはいかがでしょうか。



=== Chart系が一括で切り替えできる

@<code>{Line Chart}（折れ線グラフ）・@<code>{Area Chart}（面グラフ）・@<code>{Bar Chart}（棒グラフ）は、@<code>{Metrics & Axces}の中で
グラフの種別を切り替えられるようになりました。今までは種別を切り替えたい場合、新しくVisualizeを作成し直す必要がありました。しかし、折れ線グラフと面グラフ、どちらが
閲覧しやすいかなと迷っているときに、毎回グラフを作成し直すのは不便です。

なので、このChartの切り替え機能はとても便利でありがたいものです。@<img>{kibana02-chart01}・@<img>{kibana02-chart02}・
@<img>{kibana02-chart03}は@<code>{Chart Type}の以外は全て同じ設定を利用しています。利用しているデータ・表示期間も同じです。

//image[kibana02-chart01][折れ線グラフのとき][scale=0.7]{
//}

//image[kibana02-chart02][面グラフのとき][scale=0.7]{
//}

//image[kibana02-chart03][棒グラフのとき][scale=0.7]{
//}

グラフの種別が異なるだけで、受ける印象が変わりますね。
Kibanaの良いところは気軽にグラフを作成・削除できることです。検索の利便性を上げるために、色々オプションを試して閲覧性の高いグラフを作っていきましょう。

=== Discoverの検索窓にQueryのSyntax例が入っている

といわれても……という印象でしょうか？まず画像をみてください。

//image[kibana02-search01][検索窓にQuery Syntax例が入っているぞ][scale=0.7]{
//}

これのことです。

Kibana5までは検索用Queryを入力する窓には何も書いてありませんでした。@<code>{Kibana Discover}でインターネットの画像検索をしてみると
Kibana4・Kibana5の画面が出てきますので気になる方はどうぞ。

今までは検索用Queryがわからないとき、ブラウザを開いてQueryの記述方法を調べるか、データが存在しない時刻を表示してQueryが記載されている画面を出すしかありませんでした。
しかし、検索窓に例が記述されていれば何かしらの検索はすぐできますよね。これはとてもありがたいことです。この細やかで目立たないけれど利便性を上げる努力から、Elastic社が
Elastic Stackをより多くの人に利用してほしい、という気遣いを感じられます。
