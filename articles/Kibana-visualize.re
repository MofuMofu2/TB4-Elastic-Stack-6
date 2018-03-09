= Kibanaを使ってデータを見えるかしてみるぞい！

今日もいちにちがんばるぞい！と監視やらデータ分析やらする人は多いかと思います。しかし、テキストのデータの傾向を
テキストのまま分析するのはつらつらたんですよね。

なに？辛くないって？ほんとか？
じゃあ、この本を作るときのgit commitがいつどのくらい行われているかみてみようではありませんか。
この本の原稿はGitHub管理なのでね、ローカルにもGitリポジトリがあるわけなんですわ。

== コミットログを標準出力してみる

とはいえ、まずはGitのコミットログをファイルに出力しないことには始まりません。
まずは、@<code>{git log}コマンドでGitのコミットログを標準出力してみます。

もふちゃんのOSはmacOS High Serriaですが、GitさえインストールしてあればOS関係なく動くはずです。
コマンドはGitリポジトリが存在するディレクトリで行う必要があります。

//list[kibana01-list01][Gitのコミットログを出力する]{
git log
//}

コミットがある場合、このような形でコミットログが出力されます。

//cmd{
commit 18372016d051ad313f581244378470999c81d788
Author: MofuMofu2 <froakie002@gmail.com>
Date:   Sun Feb 18 16:07:47 2018 +0900

   [add] 本文がないとビルドがこけるので、テストファイルを追加

commit b4b18e907d5d9b2f2a93aed7cea798bd3f404232
Author: MofuMofu2 <froakie002@gmail.com>
Date:   Sun Feb 18 16:00:23 2018 +0900

   [add] 著者リストを追加

commit 14b98b2d3c7e5395e2e706f57ec546791b965df6
Author: MofuMofu2 <froakie002@gmail.com>
Date:   Sun Feb 18 15:57:54 2018 +0900
//}

この出力形式だと、閲覧するのが大変ですね。	Gitのコミットログを1行で出力する場合、@<code>{--oneline}オプションをつけます。

//list[kibana01-list02][Gitのコミットログを1行にして出力する]{
git log --oneline
//}

コマンドを実行すると、次のように出力されます。

//cmd{
a5f089c [add] Kibanaの章を追加
1837201 [add] 本文がないとビルドがこけるので、テストファイルを追加
b4b18e9 [add] 著者リストを追加
//}

あれ？なんだか出力内容が減っていますね。@<code>{--oneline}オプションをつけると、コミットのハッシュ値とコミットログ（1行目）しか出力されません。
これは少し不便です。

せめて、次の情報がないといつ、だれが、どんなコミットを作成したのかわかりません。ハッシュ値は必要ありませんが、コミットの特定ができると
変更内容を確認しやすいので情報として持っておきたいところです。

* ハッシュ値（コミットの特定の為に必要）
* Author（だれがコミットしたのか特定する為に必要）
* Authorのメールアドレス（なくてもいいけど、連絡は楽になりますよね）
* コミット時刻（いつコミットしたのかを特定する為に必要）
* コミットメッセージ（概要しりたいじゃん？）


これを実現するために@<code>{--pretty=format}オプションを利用します。@<code>{format}の引数にどんな情報を出力するのかを指定しています。

//list[kibana01-list03][Gitのコミットログを1行にし、かつ具体的な情報も出力する]{
git log  --oneline --pretty=format:"%h, %an, %aI, %f, %s "
//}

//table[kibana01-table01][--pretty:formatの引数について説明]{
引数	意味
----------
%h	ハッシュ値
%an	Author（オリジナルの成果物を作成したユーザー）
%ae	Authorのメールアドレス
%aI	Authorがコミットを作成した時刻（ISO形式）
%f	変更点の概要（変更ファイル名・修正、追加など）
%s	コミットメッセージ
//}

ちなみにコミットを作った人を出力したい場合、@<code>{%cn}のオプションを利用します。@<code>{--pretty}の具体的なオプションは@<href>{https://git-scm.com/docs/pretty-formats}で確認してください。

コミットの時刻は@<code>{ISO}形式で出力しておきます。分と秒までわかった方が時系列を整理しやすいからです。
@<code>{git log}を実行した例を記載します。

//cmd{
bcbf2e4, MofuMofu2, froakie002@gmail.com, 2018-02-18 19:16:24 +0900, add-pretty, [add] prettyオプションを利用してテストデータを作成する
//}

…これ見づらくないですか？辛くないですか？なんかグラフとか作りたくないですか？作りたいよね？
というわけで、Kibanaでこのコミットログをグラフにしてみたいと思います。なんたってこの章はKibanaの機能について解説する章だからな！

== Gitのコミットログをファイルに出力して、データの準備をする

というわけで、GitのコミットログをKibanaで閲覧してみます。まずはGitのコミットログをファイルに出力します。
そのファイルをElasticsearchに投入してKibanaでグラフを作っていきたいですからね。

Gitのコミットログをファイルに出力するには、gitコマンドの最後に@<code>{>（ファイル名）.（拡張子）}をつけます。オプションの後に半角スペースを入れてください。
それではGitのコミットログをファイルに出力してみます。

//list[kibana01-list04][Gitのコミットログをファイルに出力する]{
git log  --oneline --pretty=format:"%h, %an, %aI, %f, %s " >gitlog.json
//}

ファイルの出力先を指定したい場合、@<code>{git log オプションいろいろ >articles/log/gitlog.json}のように記述します。

@<list>{kibana01-list04}を実行すると、コミットログがファイルに出力されます。
出力結果例を下記に記載します。

//cmd{
cdbfc69, keigodasu, 2018-02-25T11:21:26+09:00, delete-unnecessary-file, delete unnecessary file
e39b32e, keigodasu, 2018-02-25T11:19:48+09:00, writing, writing
4aef633, keigodasu, 2018-02-24T13:05:42+09:00, add-sameple-source-directory, add sameple source directory
6d352ee, micci184, 2018-02-24T11:25:58+09:00, add, [add]プロダクト紹介追加
9605c33, micci184, 2018-02-21T13:13:08+09:00, add, [add]はじめにを追加
834051a, keigodasu, 2018-02-20T19:50:06+09:00, Writing, Writing
3d29902, keigodasu, 2018-02-20T19:44:29+09:00, Writing, Writing
178d741, keigodasu, 2018-02-20T19:32:10+09:00, Writing, Writing
a0f7254, keigodasu, 2018-02-20T19:18:38+09:00, Writing, Writing
bcbf2e4, MofuMofu2, 2018-02-18T19:16:24+09:00, add-pretty, [add] prettyオプションを利用してテストデータを作成する
c0a1712, MofuMofu2, 2018-02-18T19:10:17+09:00, add-npm-git-log-json, [add] npmプラグインを利用すると、git logをjson形式で出力するやつをサーバーのお仕事にできそう
//}

Authorのメールアドレスを書いておくと、他の2人から怒られそうなのでオプションから取りました。こうしてみると、それぞれ個性あるコミットログを書きますね。
では、これを本物のjsonっぽく整形していきたいと思います。

@<code>{--pretty=format}オプションの引数には、文字のベタ打ちも指定することが可能です…といっても、何をいってるのかわかりませんね。
実際の出力結果をみるとわかりやすいと思うので、まずは@<list>{kibana01-list05}を実行してみましょう。

//list[kibana01-list05][Gitのコミットログをjsonっぽく整形する]{
git log  --oneline --pretty=format:'{"commit_hash":"%h","author_name":"%an","author_date":"%aI","change_summary":"%f","subject":"%s"}' >gitlog.json
//}

実行すると、下記のようなファイルが生成されます。

//cmd{
{"commit_hash":"fd7fef2","author_name":"MofuMofu2","author_date":"2018-03-04T20:49:57+09:00","change_summary":"update","subject":"[update] コマンドと出力結果の見せ方をわけた"}
{"commit_hash":"ee03ea3","author_name":"MofuMofu2","author_date":"2018-03-04T20:49:14+09:00","change_summary":"update-list","subject":"[update] コマンドをlistにした"}
{"commit_hash":"6ca8d4d","author_name":"MofuMofu2","author_date":"2018-03-04T20:33:41+09:00","change_summary":"add","subject":"[add] この章の目的を追加して、補足を入れた"}
{"commit_hash":"89b032d","author_name":"MofuMofu2","author_date":"2018-03-04T20:23:50+09:00","change_summary":"add-Elastic-Stack","subject":"[add] Elastic Stackの基本機能を説明する章を追加"}
{"commit_hash":"d39b109","author_name":"MofuMofu2","author_date":"2018-03-04T20:23:25+09:00","change_summary":"delete","subject":"[delete] テストファイルを削除"}
{"commit_hash":"85c9d7b","author_name":"micci184","author_date":"2018-02-28T05:58:00+09:00","change_summary":"fix-logstash_beats.re","subject":"[fix]logstash_beats.re"}
{"commit_hash":"663f1c1","author_name":"micci184","author_date":"2018-02-28T05:54:22+09:00","change_summary":"fix-logstash_beats.re","subject":"[fix]logstash_beats.re"}
{"commit_hash":"f4e953c","author_name":"micci184","author_date":"2018-02-28T05:29:31+09:00","change_summary":"add-catalog.yml","subject":"[add]catalog.yml"}
{"commit_hash":"0d54c49","author_name":"micci184","author_date":"2018-02-28T05:25:47+09:00","change_summary":"Convert-md-to-Re-VIEW","subject":"Convert md to Re:VIEW"}
{"commit_hash":"05cb0dc","author_name":"micci184","author_date":"2018-02-28T05:20:03+09:00","change_summary":"typo","subject":"typo"}
{"commit_hash":"7f806cb","author_name":"micci184","author_date":"2018-02-28T04:43:48+09:00","change_summary":"add-capture","subject":"[add]capture"}
//}

jsonっぽいですね！これをKibanaで利用するサンプルデータとしたいと思います。

@<code>{git-log-to-json}というnpmパッケージを利用すると@<href>{https://www.npmjs.com/package/git-log-to-json}、Node.jsを
利用してgit logをjson形式で出力できるようです。今回は本題から外れるので扱いませんが、またどこかで記事を公開したいですねー。

== Elastic Stackの環境構築

テストデータが準備できたので、いよいよKibanaを起動しましょう。
本章のElastic Stack環境は全てzipファイルをダウンロード＆展開して構築しています。

詳細な構築方法は@<b>{Elastic Stackとは}の章を参考にしてください。
もふもふちゃんはMacに@<code>{Elastic-Stack}という名前でディレクトリを作成し、その中に各プロダクトを配置しました。

//emlist[もふもふちゃんのElastic-Stack実行環境]{
Elastic-Stack--logstash-6.2.2
							|
							-elasticsearch-6.2.2
							|
							-kibana-6.2.2-darwin-x86_64
//}

@<code>{ls}コマンドで確認した結果も参考として載せておきます。

//cmd{
~/Elastic-Stack $ ls -al
total 0
drwxr-xr-x   6 mofumofu  staff   192  3  7 11:00 .
drwxr-xr-x+ 50 mofumofu  staff  1600  3  7 10:54 ..
drwxr-xr-x@ 11 mofumofu  staff   352  2 16 19:03 elasticsearch-6.2.2
drwxr-xr-x@ 16 mofumofu  staff   512  2 17 04:20 kibana-6.2.2-darwin-x86_64
drwxr-xr-x@ 16 mofumofu  staff   512  3  7 10:51 logstash-6.2.2
//}


=== Elasticserchの起動

@<code>{elasticsearch-6.2.2}ディレクトリに移動した後、@<code>{bin/elasticsearch}でElasticsearchを起動しました。

これも、出力結果を載せておきます。ここは本題ではありませんので、解説や特別な設定は行いません。

//cmd{
~/Elastic-Stack/elasticsearch-6.2.2 $ bin/elasticsearch
[2018-03-07T17:17:49,092][INFO ][o.e.n.Node               ] [] initializing ...
[2018-03-07T17:17:49,220][INFO ][o.e.e.NodeEnvironment    ] [m3LWuZ2] using [1] data paths, mounts [[/ (/dev/disk1s1)]], net usable_space [81.9gb], net total_space [232.9gb], types [apfs]
[2018-03-07T17:17:49,221][INFO ][o.e.e.NodeEnvironment    ] [m3LWuZ2] heap size [990.7mb], compressed ordinary object pointers [true]
[2018-03-07T17:17:49,223][INFO ][o.e.n.Node               ] node name [m3LWuZ2] derived from node ID [m3LWuZ2UTR6nTATQyRi_vg]; set [node.name] to override
[2018-03-07T17:17:49,223][INFO ][o.e.n.Node               ] version[6.2.2], pid[14049], build[10b1edd/2018-02-16T19:01:30.685723Z], OS[Mac OS X/10.13.3/x86_64], JVM[Oracle Corporation/Java HotSpot(TM) 64-Bit Server VM/1.8.0_45/25.45-b02]
[2018-03-07T17:17:49,224][INFO ][o.e.n.Node               ] JVM arguments [-Xms1g, -Xmx1g, -XX:+UseConcMarkSweepGC, -XX:CMSInitiatingOccupancyFraction=75, -XX:+UseCMSInitiatingOccupancyOnly, -XX:+AlwaysPreTouch, -Xss1m, -Djava.awt.headless=true, -Dfile.encoding=UTF-8, -Djna.nosys=true, -XX:-OmitStackTraceInFastThrow, -Dio.netty.noUnsafe=true, -Dio.netty.noKeySetOptimization=true, -Dio.netty.recycler.maxCapacityPerThread=0, -Dlog4j.shutdownHookEnabled=false, -Dlog4j2.disable.jmx=true, -Djava.io.tmpdir=/var/folders/5z/1qmk32x17pn9zv80fk26bvsw0000gn/T/elasticsearch.bvcXHDjg, -XX:+HeapDumpOnOutOfMemoryError, -XX:+PrintGCDetails, -XX:+PrintGCDateStamps, -XX:+PrintTenuringDistribution, -XX:+PrintGCApplicationStoppedTime, -Xloggc:logs/gc.log, -XX:+UseGCLogFileRotation, -XX:NumberOfGCLogFiles=32, -XX:GCLogFileSize=64m, -Des.path.home=/Users/mofumofu/Elastic-Stack/elasticsearch-6.2.2, -Des.path.conf=/Users/mofumofu/Elastic-Stack/elasticsearch-6.2.2/config]
[2018-03-07T17:17:50,813][INFO ][o.e.p.PluginsService     ] [m3LWuZ2] loaded module [aggs-matrix-stats]
[2018-03-07T17:17:50,813][INFO ][o.e.p.PluginsService     ] [m3LWuZ2] loaded module [analysis-common]
[2018-03-07T17:17:50,813][INFO ][o.e.p.PluginsService     ] [m3LWuZ2] loaded module [ingest-common]
[2018-03-07T17:17:50,814][INFO ][o.e.p.PluginsService     ] [m3LWuZ2] loaded module [lang-expression]
[2018-03-07T17:17:50,814][INFO ][o.e.p.PluginsService     ] [m3LWuZ2] loaded module [lang-mustache]
[2018-03-07T17:17:50,814][INFO ][o.e.p.PluginsService     ] [m3LWuZ2] loaded module [lang-painless]
[2018-03-07T17:17:50,814][INFO ][o.e.p.PluginsService     ] [m3LWuZ2] loaded module [mapper-extras]
[2018-03-07T17:17:50,814][INFO ][o.e.p.PluginsService     ] [m3LWuZ2] loaded module [parent-join]
[2018-03-07T17:17:50,814][INFO ][o.e.p.PluginsService     ] [m3LWuZ2] loaded module [percolator]
[2018-03-07T17:17:50,814][INFO ][o.e.p.PluginsService     ] [m3LWuZ2] loaded module [rank-eval]
[2018-03-07T17:17:50,815][INFO ][o.e.p.PluginsService     ] [m3LWuZ2] loaded module [reindex]
[2018-03-07T17:17:50,815][INFO ][o.e.p.PluginsService     ] [m3LWuZ2] loaded module [repository-url]
[2018-03-07T17:17:50,815][INFO ][o.e.p.PluginsService     ] [m3LWuZ2] loaded module [transport-netty4]
[2018-03-07T17:17:50,815][INFO ][o.e.p.PluginsService     ] [m3LWuZ2] loaded module [tribe]
[2018-03-07T17:17:50,816][INFO ][o.e.p.PluginsService     ] [m3LWuZ2] no plugins loaded
[2018-03-07T17:17:55,806][INFO ][o.e.d.DiscoveryModule    ] [m3LWuZ2] using discovery type [zen]
[2018-03-07T17:17:56,654][INFO ][o.e.n.Node               ] initialized
[2018-03-07T17:17:56,654][INFO ][o.e.n.Node               ] [m3LWuZ2] starting ...
[2018-03-07T17:17:56,986][INFO ][o.e.t.TransportService   ] [m3LWuZ2] publish_address {127.0.0.1:9300}, bound_addresses {[::1]:9300}, {127.0.0.1:9300}
[2018-03-07T17:18:00,133][INFO ][o.e.c.s.MasterService    ] [m3LWuZ2] zen-disco-elected-as-master ([0] nodes joined), reason: new_master {m3LWuZ2}{m3LWuZ2UTR6nTATQyRi_vg}{cbEG0uFuRpWyWagkgCvI9A}{127.0.0.1}{127.0.0.1:9300}
[2018-03-07T17:18:00,139][INFO ][o.e.c.s.ClusterApplierService] [m3LWuZ2] new_master {m3LWuZ2}{m3LWuZ2UTR6nTATQyRi_vg}{cbEG0uFuRpWyWagkgCvI9A}{127.0.0.1}{127.0.0.1:9300}, reason: apply cluster state (from master [master {m3LWuZ2}{m3LWuZ2UTR6nTATQyRi_vg}{cbEG0uFuRpWyWagkgCvI9A}{127.0.0.1}{127.0.0.1:9300} committed version [1] source [zen-disco-elected-as-master ([0] nodes joined)]])
[2018-03-07T17:18:00,174][INFO ][o.e.h.n.Netty4HttpServerTransport] [m3LWuZ2] publish_address {127.0.0.1:9200}, bound_addresses {[::1]:9200}, {127.0.0.1:9200}
[2018-03-07T17:18:00,174][INFO ][o.e.n.Node               ] [m3LWuZ2] started
[2018-03-07T17:18:00,194][INFO ][o.e.g.GatewayService     ] [m3LWuZ2] recovered [0] indices into cluster_state
[2018-03-07T17:20:57,666][INFO ][o.e.c.m.MetaDataCreateIndexService] [m3LWuZ2] [logstash-2018.03.07] creating index, cause [auto(bulk api)], templates [logstash], shards [5]/[1], mappings [_default_]
[2018-03-07T17:20:58,218][INFO ][o.e.c.m.MetaDataMappingService] [m3LWuZ2] [logstash-2018.03.07/WBFN2jXwR16CDprJIlIq-w] create_mapping [doc]
//}


=== Logstashの起動

Kibanaで閲覧するGitのコミットログをElasticsearchに投入するため、Logstashを利用します。
Kibana5.4（beta版）ではKibanaのUIからCSVをElasticsearchに投入できる機能があったのですが、いつの間にか廃止されていました…。なので、
（仕方なく）Logstashを利用します。この辺はこだわりがありませんので、何らかの形でElasticsearchにデータを投入しましょう。

もふもふちゃんは@<code>{config/conf.d}フォルダに@<code>{gitlog-logstash.conf}を作成しました。

//list[kibana01-list06][gitlog-logstash.conf]{
input {
		file {
			path => "/Users/mofumofu/log/*.json"
			tags => "git-log"
		}
}

filter {
	json {
		source => "message"
	}
}

output {
	stdout { codec => rubydebug }
	elasticsearch { }
}
//}

解説するほどの設定はありませんが、いくつか補足します。

動作確認をしたかったので、念のため@<code>{stdout}で標準出力をするように設定しています。
また、Elasticserchはローカル環境で起動したものを利用するため、IPアドレスなどは設定していません。
デフォルトの設定は@<code>{localhost}のElasticsearchを参照するようになっているからです。

@<code>{logstash.conf}を配置後、@<code>{bin/logstash -f config/conf.d/gitlog-logstash.conf}でLogstashを起動します。
このとき、Elasticsearchと同様に@<code>{logstash-6.2.2}ディレクトリに移動してからコマンドを実行します。もふちゃんはiTerm2を利用しているので、別タブを開いて起動しました。
@<code>{-f コンフィグの配置場所}でちゃんとファイルパス、ファイル名を指定しないと「configがないよーん」とエラーになりLogstashを起動できません。
そこはトラブルになりやすいので気をつけたらいいと思います。

これも参考として、出力結果を載せておきます。

//cmd{
~/Elastic-Stack/logstash-6.2.2 $ bin/logstash -f config/conf.d/gitlog-logstash.conf
//}

#@#標準出力結果入れる

=== Kibanaの起動

データがElasticsearchに投入できたので、最後にKibanaを起動します。
これも他2プロダクトと同様に、@<code>{kibana-6.2.2-darwin-x86_64}ディレクトリに移動後、@<code>{bin/kibana}でKibanaを起動します。
@<code>{Server running at http://kibana.ymlで記載したIPアドレス:ポート番号}と出力されれば、正常に起動できています。

もふちゃんは@<code>{kibana.yml}を修正していないため、@<code>{localhost:5601}でKibanaは起動します。

//cmd{
~ $ cd Elastic-Stack/
~/Elastic-Stack $ cd kibana-6.2.2-darwin-x86_64/
~/Elastic-Stack/kibana-6.2.2-darwin-x86_64 $ bin/kibana
  log   [08:19:01.911] [info][status][plugin:kibana@6.2.2] Status changed from uninitialized to green - Ready
  log   [08:19:02.045] [info][status][plugin:elasticsearch@6.2.2] Status changed from uninitialized to yellow - Waiting for Elasticsearch
  log   [08:19:02.623] [info][status][plugin:timelion@6.2.2] Status changed from uninitialized to green - Ready
  log   [08:19:02.632] [info][status][plugin:console@6.2.2] Status changed from uninitialized to green - Ready
  log   [08:19:02.652] [info][status][plugin:metrics@6.2.2] Status changed from uninitialized to green - Ready
  log   [08:19:02.725] [info][listening] Server running at http://localhost:5601
  log   [08:19:02.823] [info][status][plugin:elasticsearch@6.2.2] Status changed from yellow to green - Ready
//}
