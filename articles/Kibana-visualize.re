= Kibanaを使ってデータを可視化する

サーバー監視やデータ分析をする際、テキストのデータの傾向を
テキストのまま分析するのは辛いものです。
この章では、この書籍を制作する際のgit commitがいつ・どのくらい行われているかについて分析し、可視化してみます。

== コミットログを標準出力してみる

まずはGitのコミットログをファイルに出力します。
@<code>{git log}コマンドでGitのコミットログを標準出力してみます。

筆者のOSはmacOS High Serriaですが、GitさえインストールしてあればOSの関係なく動くはずです。
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
//}

この出力形式では閲覧するのが大変です。Gitのコミットログを1行で出力する場合、@<code>{--oneline}オプションをつけます。

//list[kibana01-list02][Gitのコミットログを1行にして出力する]{
git log --oneline
//}

コマンドを実行すると、次のように出力されます。

//cmd{
a5f089c [add] Kibanaの章を追加
1837201 [add] 本文がないとビルドがこけるので、テストファイルを追加
b4b18e9 [add] 著者リストを追加
//}

出力内容が減っていることがわかります。@<code>{--oneline}オプションを利用すると、コミットのハッシュ値とコミットログ（1行目）しか出力されません。
これは少し不便です。

いつ、だれが、どんなコミットを作成したのかわからないと、各々の作業進捗を把握することはできません。ハッシュ値は作業進捗の把握に必須ではありませんが、
ハッシュ値を取得しておけば、どのコミットが具体的な作業内容に紐づくのか把握することができます。

 * ハッシュ値（コミットの特定のために必要）
 * Author（だれがコミットしたのか特定するために必要）
 * Authorのメールアドレス（連絡するための項目）
 * コミット時刻（いつコミットしたのかを特定する為に必要）
 * コミットメッセージ（概要を知るための項目）


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

コミットを作った人を出力したい場合、@<code>{%cn}のオプションを利用します。@<code>{--pretty}の具体的なオプションは@<href>{https://git-scm.com/docs/pretty-formats}で確認してください。

コミットの時刻は@<code>{ISO}形式で出力します。分と秒までわかった方が時系列を整理しやすいからです。
@<code>{git log}を実行した例を記載します。

//cmd{
bcbf2e4, MofuMofu2, froakie002@gmail.com, 2018-02-18 19:16:24 +0900, add-pretty, [add] prettyオプションを利用してテストデータを作成する
//}

これでも作業内容を把握することは可能ですが、できればグラフでいつ・だれがコミットを作成したのか把握したいですよね。
よって、Kibanaでこのコミットログをグラフにしてみたいと思います。

== Gitのコミットログをファイルに出力して、データの準備をする

GitのコミットログをKibanaで閲覧するために、まずはGitのコミットログをファイルに出力します。
そのファイルをElasticsearchに投入してKibanaでグラフを作ります。

Gitのコミットログをファイルに出力するには、gitコマンドの最後に@<code>{>（ファイル名）.（拡張子）}をつけます。オプションの後に半角スペースを入れてください。
それではGitのコミットログをファイルに出力してみます。

//list[kibana01-list04][Gitのコミットログをファイルに出力する]{
git log  --oneline --pretty=format:"%h, %an, %aI, %f, %s " >gitlog.json
//}

ファイルの出力先を指定したい場合、@<code>{git log オプションいろいろ >articles/log/gitlog.json}のように記述します。

@<list>{kibana01-list04}を実行すると、コミットログがファイルに出力されます。
出力結果例は次のようなものです。

//cmd{
cdbfc69, keigodasu, 2018-02-25T11:21:26+09:00, delete-unnecessary-file, delete unnecessary file
e39b32e, keigodasu, 2018-02-25T11:19:48+09:00, writing, writing
4aef633, keigodasu, 2018-02-24T13:05:42+09:00, add-sameple-source-directory
6d352ee, micci184, 2018-02-24T11:25:58+09:00, add, [add]プロダクト紹介追加
9605c33, micci184, 2018-02-21T13:13:08+09:00, add, [add]はじめにを追加
834051a, keigodasu, 2018-02-20T19:50:06+09:00, Writing, Writing
3d29902, keigodasu, 2018-02-20T19:44:29+09:00, Writing, Writing
178d741, keigodasu, 2018-02-20T19:32:10+09:00, Writing, Writing
a0f7254, keigodasu, 2018-02-20T19:18:38+09:00, Writing, Writing
bcbf2e4, MofuMofu2, 2018-02-18T19:16:24+09:00, add-pretty, [add] prettyオプションを利用してテストデータを作成する
c0a1712, MofuMofu2, 2018-02-18T19:10:17+09:00, add-npm-git-log-json, [add] npmプラグインを利用すると、git logをjson形式で出力するやつをサーバーのお仕事にできそう
//}

Authorのメールアドレスは誌面に掲載する都合上オプションから取り除いています。
では、これを本物のJSONのように整形していきたいと思います。

@<code>{--pretty=format}オプションの引数には、文字のベタ打ちも指定することが可能です。
実際の出力結果をみるために、まずは@<list>{kibana01-list05}を実行します。

//list[kibana01-list05][Gitのコミットログをjsonっぽく整形する]{
git log  --oneline --pretty=format:'{"commit_hash":"%h","author_name":"%an",
（ページの都合で改行）
"author_date":"%aI","change_summary":"%f","subject":"%s"}'
（ページの都合で改行）
>gitlog.json
//}

実行すると、次のようなファイルが生成されます。紙面の都合上、途中で改行しています。

//cmd{
{"commit_hash":"fd7fef2","author_name":"MofuMofu2",
（ページの都合で改行）
"author_date":"2018-03-04T20:49:57+09:00",
"change_summary":"update","subject":"[update] コマンドと出力結果の見せ方をわけた"}
//}

JSON形式でログが出力されました。これをKibanaで利用するサンプルデータとしたいと思います。

@<code>{git-log-to-json}というnpmパッケージを利用すると（@<href>{https://www.npmjs.com/package/git-log-to-json}）、Node.jsを
利用してgit logをJSON形式で出力できるようです。今回は本題から外れるので扱いませんが、活用してみてください。

== Elastic Stackの環境構築

テストデータが準備できたので、いよいよKibanaを起動しましょう。
本章のElastic Stack環境は全てzipファイルをダウンロード＆展開して構築しています。

まずMacに@<code>{Elastic-Stack}という名前でディレクトリを作成し、その中に各プロダクトを配置しました。

//emlist[筆者のElastic-Stack実行環境]{
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


=== Elasticsearchの起動

@<code>{elasticsearch-6.2.2}ディレクトリに移動した後、@<code>{bin/elasticsearch}でElasticsearchを起動します。

=== Logstashの起動

Kibanaで閲覧するGitのコミットログをElasticsearchに投入するため、Logstashを利用しました。
このユースケースでは、Elasticsearchにデータを投入する手段にLogstashを利用しています。しかし、
他のプロダクトやElasticsearchのAPIなどを利用してデータを投入しても問題はありません。

筆者は@<code>{config/conf.d}フォルダに@<code>{gitlog-logstash.conf}を作成しました。

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


動作確認のために、念のため@<code>{stdout}で標準出力をするように設定しています。
また、Elasticserchはローカル環境で起動したものを利用するため、IPアドレスなどは設定していません。
デフォルトの設定は@<code>{localhost}のElasticsearchを参照するようになっているからです。

@<code>{logstash.conf}を配置後、@<code>{bin/logstash -f config/conf.d/gitlog-logstash.conf}でLogstashを起動します。
このとき、Elasticsearchと同様に@<code>{logstash-6.2.2}ディレクトリに移動してからコマンドを実行します。筆者はiTerm2を利用しているので、別タブを開いて起動しました。
@<code>{-f コンフィグの配置場所}でファイルパス、ファイル名を指定しないと「configがない」とエラーになりLogstashを起動できません。
ここはトラブルになりやすいので気をつけるとよいでしょう。

=== Kibanaの起動

Elasticsearchにデータが投入できたので、Kibanaを起動します。
これも他2プロダクトと同様に、@<code>{kibana-6.2.2-darwin-x86_64}ディレクトリに移動後、@<code>{bin/kibana}でKibanaを起動します。
@<code>{Server running at http://kibana.ymlで記載したIPアドレス:ポート番号}と出力されれば、正常に起動できています。

筆者は@<code>{kibana.yml}を修正していないため、@<code>{localhost:5601}でKibanaが起動します。


この状態でブラウザから@<href>{http://localhost:5601/}にアクセスすると、@<img>{kibana01-img01}のような画面が見えているはずです。

//image[kibana01-img01][Kibana（ver6.2）の画面][scale=0.7]{
//}

これで準備はできました。

== Kibanaを使ってGitのコミット状況を閲覧する

では、早速Gitのコミットログ（以降git logとします）をグラフにしていきましょう。
まずはKibana（@<href>{http://localhost:5601}）にアクセスします。@<code>{kibana.yml}でURLを変更していた場合、
自分で設定したURLへアクセスしてください。

アクセスすると、@<img>{kibana01-img01}が見えていますね。まずは画面左端にある歯車アイコンを押して@<code>{Management}画面を開きましょう。

=== 利用するindexの設定を行う

Elasticsearchは@<code>{index}にデータを保存しています。Kibanaでグラフを作るときに、どの@<code>{index}を参照すればよいか
はじめに設定する必要があります。

//image[kibana01-img02][Kibanaが参照するindexを設定する][scale=0.7]{
//}

画面下側にindexの名前が出てきます。コピー＆ペーストで@<code>{Index pattern}にindex名を入れてしまいましょう。
index名の指定をするときは、@<code>{*（アスタリスク）}を利用することができます。たとえば@<code>{logstash-*}と設定すれば
@<code>{logstash-}で始まるindexを全て参照することができます。

デフォルトでは、LogstashからデータをElasticsearchに連携するときに@<code>{logstash-日付}としてindexを作成します。
なので、Logstash側でindexを指定していない場合、@<code>{logstash-*}をKibanaから参照するようにしておけば問題ありません。

次に、どのfieldを時刻として参照するか設定します。

//image[kibana01-img03][どのfieldを時刻として参照するか設定する][scale=0.7]{
//}

@<code>{@timestamp}を選択すると、LogstashがデータをElasticsearchに連携した時刻を基準としてデータを閲覧することになります。
今回はいつGitにコミットが作成されたかを閲覧したいので、@<code>{author_date}を時刻として参照するようにします。

== Discoverでgit logの様子を観察する

//image[kibana01-img04][Discoverの画面に遷移した状態]{
//}

画面左端にあるコンパスのアイコンを押すと、@<code>{Discover}の画面に遷移します。@<code>{Discover}ではElasticsearchに保存されているデータを
直接参照することが可能です。画面上部のグラフは、いつ・どのくらいのデータがElasticsearchに保存されたかを示しています。ここで先ほどindexの設定時に指定した時刻を利用します。

画面右上の時計マークでは、表示するデータの期間を指定しています。たとえば@<img>{kibana01-img04}では、時刻が@<code>{Last 15 minutes}と設定されています。
この場合、@<b>{今の時刻から}15分前までにコミットがあったデータ（＝@<code>{author_date}の時刻が現在から15分前のもの）を閲覧する状態となっています。

条件に当てはまるデータが存在しない場合、@<img>{kibana01-img04}のようにデータが存在しないことを示す画面が表示されます。
この場合、時計マークをクリックして時刻の範囲を変更しましょう。時刻を広めにとると何かしらのデータが表示されるはずです。それでもダメであれば、Elasticsearchにデータが
保存されていない可能性があります。データの連係がきちんとできているかもう一度見直しましょう。

//image[kibana01-img05][時刻を調整してgit logがDiscover画面に表示された][scale=0.7]{
//}

データの詳細を閲覧するためには、データの横にある@<code>{▶︎}をクリックします。
JSONのfieldごとにデータが別れて表示されるので、どのfieldに何のデータが保存されたかを確認することが可能です。

//image[kibana01-img06][▶︎を押してデータの詳細を閲覧する][scale=0.7]{
//}

基本的なデータの参照方法がわかったところで、いよいよグラフを作成していきたいと思います。

== Visualizeで進捗を観察する

では、早速新しいグラフを作成します。画面左端の棒グラフアイコンをクリックして@<code>{Visualize}を開きましょう。
開くと@<img>{kibana01-img07}のように、グラフを選択する画面が開きます。
すでにグラフが存在すれば、ここから詳細を閲覧することができますが、今回は何もグラフが存在しないので新しくグラフを作ります
@<code>{Create a visualization}をクリックしてグラフを作成しましょう。

//image[kibana01-img07][グラフが存在しないので新しく作る][scale=0.7]{
//}

@<code>{Create a visualization}をクリックすると、@<img>{kibana01-img08}のようにグラフの種別を選択する画面が出てきます。
まずは基本の折れ線グラフを作成してみましょう。

//image[kibana01-img08][グラフ種別の選択]{
//}


@<code>{Line}を選択すると、どのindexデータを利用するかを指定する画面になります。indexの設定画面で指定した名前をクリックして
次に進みましょう。

=== Line Chartを作成する

//image[kibana01-img10][Visualizeの初期画面][scale=0.7]{
//}

これでは何も表示されていませんね。次の順序でグラフを作成したいと思います。

 1. X軸（横軸）の設定を行う
 2. Y軸（縦軸）の設定を行う
 3. コミッターごとにグラフの線を分ける
 4. 見た目をいい感じに整える

==== X軸（横軸）の設定を行う

折れ線グラフなので、時系列でどのようにコミット数が遷移しているか分かると気持ちがいいですよね。というわけで
X軸の基準を時間に変更したいと思います。

@<code>{Buckets}の@<code>{X-Axis}をクリックして、詳細画面を開きましょう。
@<code>{Aggregation}でどんな基準をX軸にするのか決定します。Aggregationは集合という意味ですから、
どんなデータの集まりをグラフにするのかを決定するという意味だとわかりますね。

今回はコミットの時間をX軸にしたいので、@<code>{Date Histogram}を選択します。すると、
自動で自分が設定した時間軸が@<code>{Field}に入ってきます。もちろん、ここで時間軸として利用するfieldを
変更することも可能です。

//image[kibana01-img11][X軸をDate Histogramに][scale=0.7]{
//}

設定を変更して画面左上の@<code>{▶︎}ボタンをクリックすると、グラフが@<img>{kibana01-img11}のように変化しました。
このように、設定を変更したら@<code>{▶︎}を押さないと変更が反映されません。画面全体をリロードすると、もう一度設定をやり直す
ことになります。

@<code>{Interval}はデータをプロットする間隔を指定します。最初は@<code>{Auto}にしておいて、後から自分の好みで
設定しなおすとよいでしょう。

==== Y軸（縦軸）の設定を行う

今度はY軸の設定を行います。今回は「いつどのくらいコミットがあったかをみたい」ことが目的なので、
コミット数が時系列にプロットされていることが必要です。

ただ、データの平均を見たいときは困りますね。@<code>{Metrics}の@<code>{Aggregation}をクリックすると
今度はどんな方法でデータの数を数えるか変更することができます。デフォルトは@<code>{Count}なのでデータの数を縦にプロットしますが、
@<code>{Average}に変更すると、データの平均をプロットすることが可能です。SQLのように、MAX・MIN・Sumといった演算をすることも可能です。
ただし、これら数値を扱うような設定は、indexに保存されているデータに数値型のものがないと利用できません。今回は文字列型のデータばかりですから、
@<code>{Count}を利用することにしましょう。

//image[kibana01-img12][Y軸の設定を行うMetrics][scale=0.7]{
//}

==== コミッターごとにグラフの線を分ける

現時点でも日によってコミット量に差がある、ということがわかって面白いのですが、コミッターごとにグラフを分割できたほうが
もっと面白いですよね。誰がサボってる！とか、駆け込みコミット型ですね！などが分かれば進捗が管理しやすくなります。

というわけで、折れ線グラフをコミッターごとに分割しましょう。

@<code>{Buckets}の下側にある@<code>{Add sub-buckets}をクリックします。@<code>{Select buckets type}の画面が出てきて
次の2種類が選択できるようになります。

 * Split Series
 * Split Chart

fieldの値ごとに折れ線グラフを分けて表示したい場合は@<code>{Split Series}を、1つの折れ線グラフをfieldの値ごとに分割したいときは@<code>{Split Chart}を利用します。
今回はコミッターごとに折れ線グラフを分けたいので@<code>{Split Series}を編集していきます。どちらも編集の流れは同じなので、@<code>{Split Chart}を利用したい人も
これ以降の編集の流れを参照してみてください。

@<code>{Sub Aggregation}ではグラフを分割する基準を決めることができます。今回はgit logの@<code>{author_name}で分割したいので、
@<code>{Terms}を指定してfieldを用いてグラフを分割できるように設定します。

@<code>{Field}で実際のfield名を指定します。@<img>{kibana01-img13}では@<code>{author_name.keyword}と記載されていますが、
field名の後にはデータの型が記載されています。プログラミング言語と違い、文字列型は@<code>{keyword}と記載されます。アイコンは「t」と書いてあるので
textだと分かります。

@<code>{Order By}ではグラフとして表示する@<code>{author_name}は上位5名までと設定しています。@<code>{Descending}は上位XX、
@<code>{Ascending}は下位XX名となります。

Order Byというと、SQLのORDER BY句を連想しますが、KibanaのOrder Byはソートに加え、指定した数しかグラフを表示してくれません。
たとえば今回の場合、もし10人コミッターがいたとしても@<img>{kibana01-img13}の設定ではコミット数上位5名しか表示されません。
このように、Kibanaのグラフを作成するときは自分が可視化したいデータの特性をちゃんと把握しておくことが重要になります。

//image[kibana01-img13][コミッターごとに折れ線グラフを表示]{
//}

=== できたグラフを観察する

ちょっと時間軸も長めに設定してみました（2ヶ月分くらいにしています）。筆者ちゃんが散々「2月中に初稿を書けよ！」と
脅したせいかはわかりませんが、2月末だけ明らかにコミット量が増えています。そのあとは個人の好き好きに修正をかけたりしています。

これを@<code>{Metrics & Axces}オプションから棒グラフ（@<code>{bar}）にしてみると、@<img>{kibana01-img14}のようになります。
やはりコミット数の推移を見たいのであれば、折れ線グラフのようなプロット型の物を利用した方がわかりやすいですね。

//image[kibana01-img14][コミッターごとのコミット数を棒グラフにしてみた]{
//}

グラフの保存は画面右上の@<code>{Save}から行うことができます。好きな名前をいれて保存しておきましょう。
保存しない場合、画面を閉じてしまったら設定は全部消えます。もう一度最初から作り直しです。

== この章のまとめ

Kibanaを利用すると、データという文字の情報をグラフィカルに分析することができる、ということを体感いただけましたか？

Kibanaの良い点を活かすために必要なことは、次の2点です。

 * なるべく元データをLogstashなどデータ収集ツールが扱いやすい形に加工しておくこと
 * データの特性・内容を把握しグラフを作成すること

特に、データのパースにはそれなりのリソースを消費しますから、なるべくならデータはJSONなどの階層がある形式にしておきたいものです。
