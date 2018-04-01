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
コマンドはGitリポジトリが存在するディレクトリで行う必要があります。なに、そんなのは常識だって？すいませんでしたね。

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

Authorのメールアドレスを書いておくと、他の著者2人から怒られそうなのでオプションから取りました。こうしてみると、それぞれ個性あるコミットログを書きますね。
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
#@#文章できたらリンクを貼る

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


=== Kibanaの起動

データがElasticsearchに投入できたので、最後にKibanaを起動します。
これも他2プロダクトと同様に、@<code>{kibana-6.2.2-darwin-x86_64}ディレクトリに移動後、@<code>{bin/kibana}でKibanaを起動します。
@<code>{Server running at http://kibana.ymlで記載したIPアドレス:ポート番号}と出力されれば、正常に起動できています。

もふちゃんは@<code>{kibana.yml}を修正していないため、@<code>{localhost:5601}でKibanaは起動します。


この状態でブラウザから@<herf>{http://localhost:5601/}にアクセスすると、@<img>{kibana01-img01}のような画面が見えているはずです。

//image[kibana01-img01][Kibana（ver6.2）の画面]{
//}

これで準備はできました。
