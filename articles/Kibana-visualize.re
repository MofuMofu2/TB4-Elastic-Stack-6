= Kibanaを使ってデータを見えるかしてみるぞい！

今日もいちにちがんばるぞい！と監視やらデータ分析やらする人は多いかと思います。しかし、テキストのデータの傾向を
テキストのまま分析するのはつらつらたんですよね。

なに？辛くないって？ほんとか？
じゃあ、この本を作るときのgit commitがいつどのくらい行われているかみてみようではありませんか。
この本の原稿はGitHub管理なのでね、ローカルにもGitリポジトリがあるわけなんですわ。

== コミットログを標準出力してみる

とはいえ、まずはGitのコミットログをファイルに出力しないことには始まりません。
まずは、@@<code>{git log}コマンドでGitのコミットログを標準出力してみます。

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

あれ？なんだか出力内容が減っていますね。@@<code>{--oneline}オプションをつけると、コミットのハッシュ値とコミットログ（1行目）しか出力されません。
これは少し不便です。

せめて、次の情報がないといつ、だれが、どんなコミットを作成したのかわかりません。ハッシュ値は必要ありませんが、コミットの特定ができると
変更内容を確認しやすいので情報として持っておきたいところです。

* ハッシュ値（コミットの特定の為に必要）
* Author（だれがコミットしたのか特定する為に必要）
* Authorのメールアドレス（なくてもいいけど、連絡は楽になりますよね）
* コミット時刻（いつコミットしたのかを特定する為に必要）
* コミットメッセージ（概要しりたいじゃん？）


これを実現するために@<code>{--pretty}オプションを利用します。@@<code>{format}の引数にどんな情報を出力するのかを指定しています。

//list[kibana01-list03][Gitのコミットログを1行にし、かつ具体的な情報も出力する]{
git log  --oneline --pretty=format:"%h, %an, %ae, %ad, %s " --date="iso"
//}

//table[kibana01-table01][--pretty:formatの引数について説明]{
引数	意味
----------
%h	ハッシュ値
%an	Author（コミットを作成したユーザー）
%ae	Authorのメールアドレス
%ad	Authorがコミットを作成した時刻
%s	コミットメッセージ
//}


コミットの時刻は@<code>{iso}形式で出力しておきます。分と秒までわかった方が時系列を整理しやすいからです。
ちなみに、@@<code>{--pretty}の具体的なオプションは@<href>{https://git-scm.com/docs/pretty-formats}で確認できます。

//cmd{
e9c1356, MofuMofu2, froakie002@gmail.com, 2018-02-18 18:00:39 +0900, [add] git logを出力する
a5f089c, MofuMofu2, froakie002@gmail.com, 2018-02-18 17:40:29 +0900, [add] Kibanaの章を追加
1837201, MofuMofu2, froakie002@gmail.com, 2018-02-18 16:07:47 +0900, [add] 本文がないとビルドがこけるので、テストファイルを追加
b4b18e9, MofuMofu2, froakie002@gmail.com, 2018-02-18 16:00:23 +0900, [add] 著者リストを追加
14b98b2, MofuMofu2, froakie002@gmail.com, 2018-02-18 15:57:54 +0900, [fix] ファイル名をwercker出力名に合わせる
c201d78, MofuMofu2, froakie002@gmail.com, 2018-02-18 15:56:35 +0900, ひつようなファイルをコミットだよ
4e62dae, MofuMofu2, froakie002@gmail.com, 2018-02-18 15:30:47 +0900, [fix] reviewファイルの前に半角ハイフンがなかったので修正
//}

…これ、見づらくないですか？辛くないですか？なんかグラフとか作りたくないですか？作りたいよね？
というわけで、Kibanaでこのコミットログをグラフにしてみたいと思います。なんたってこの章はKibanaの機能について解説する章だからな！

=== コミットログをファイルに出力して、データの準備をする

というわけで、GitのコミットログをKibanaで閲覧してみます。もふちゃんはKibanaとElasticsearchのzipファイルを展開して
環境を作りました。環境の作成方法は@@<b>{Elastic Stackとは}の章を参考にしてください。あ、もふちゃんの商業本（技術書典シリーズから出てるやつ）を買うてもええんやで！（しつこいくらいのすてま）

Gitのコミットログをファイルに出力するには、gitコマンドの最後に@<code>{>（ファイル名）.（拡張子）}をつけます。オプションの後に半角スペースを入れてください。
しかし、このままファイル出力を行うとElasticsearchにデータを投入する前にLogstashで加工が必要です。


Kibanaを手っ取り早く試したいのに、わざわざデータを加工するのはしんどいですよね。なので、コミットログをjsonで出力してみたいと思います。
まずはコミットログをjsonっぽく出力してみます。@<code>{pretty}オプションは普通の文字ベタ打ちと組み合わせて利用することができます。


//cmd{
$ git log  --oneline --pretty=format:"{"commit_hash":%h,"author_name":%an,"author_email":%ae,"author_date":%ad,"subject":%s} " --date="iso"
{commit_hash:020670e,author_name:keigodasu,author_email:keigodasu0524@yahoo.co.jp,author_date:2018-02-26 21:15:22 +0900,subject:edited alias part}
{commit_hash:1ebe8d1,author_name:keigodasu,author_email:keigodasu0524@yahoo.co.jp,author_date:2018-02-26 20:56:47 +0900,subject:writing error handling}
{commit_hash:0cffae6,author_name:keigodasu,author_email:keigodasu0524@yahoo.co.jp,author_date:2018-02-26 20:37:14 +0900,subject:writing}
{commit_hash:b39f3d6,author_name:keigodasu,author_email:keigodasu0524@yahoo.co.jp,author_date:2018-02-26 20:02:50 +0900,subject:writing}
{commit_hash:90a97d2,author_name:keigodasu,author_email:keigodasu0524@yahoo.co.jp,author_date:2018-02-26 19:48:56 +0900,subject:writing}
{commit_hash:934ef39,author_name:keigodasu,author_email:keigodasu0524@yahoo.co.jp,author_date:2018-02-26 18:58:41 +0900,subject:writing}
{commit_hash:52ce336,author_name:micci184,author_email:micci184@gmail.com,author_date:2018-02-26 10:24:54 +0900,subject:[add]logstash.md}
{commit_hash:03850a1,author_name:micci184,author_email:micci184@gmail.com,author_date:2018-02-26 10:21:50 +0900,subject:Merge branch 'master' of https://github.com/MofuMofu2/TB4-Elastic-Stack-6}
{commit_hash:7067f94,author_name:micci184,author_email:micci184@gmail.com,author_date:2018-02-26 10:19:54 +0900,subject:[add]logstash.md}
{commit_hash:33937bc,author_name:keigodasu,author_email:keigodasu0524@yahoo.co.jp,author_date:2018-02-25 13:25:53 +0900,subject:add deleting sammple}
{commit_hash:22efb5a,author_name:keigodasu,author_email:keigodasu0524@yahoo.co.jp,author_date:2018-02-25 13:17:55 +0900,subject:add indexing sammple}
{commit_hash:1d2701d,author_name:keigodasu,author_email:keigodasu0524@yahoo.co.jp,author_date:2018-02-25 12:57:22 +0900,subject:writing}
{commit_hash:c57d160,author_name:keigodasu,author_email:keigodasu0524@yahoo.co.jp,author_date:2018-02-25 12:03:37 +0900,subject:add description of data types}
//}

どうでしょうか？手書きでもjsonっぽくなりました。では、実際にこれをjsonファイルに出力してみたいと思います。

@<code>{git log  --oneline --pretty=format:'{"commit_hash":"%h","author_name":"%an","author_email":"%ae","author_date":"%aI","subject":"%s"\},' --date="iso" >articles/log/gitLog.json}と記述してみましょう。

#@# %aIにした、ISO形式で出した方があとでKibanaしたときに楽そうだよね

@<code>{git-log-to-json}というnpmパッケージを利用すると@<href>{https://www.npmjs.com/package/git-log-to-json}、Node.jsを
利用してgit logをjson形式で出力できるようです。今回は本題から外れるので扱いませんが、またどこかで記事を公開したいですねー。

@<code>{pretty}オプションの引数ですが、git logの情報は決められた形式以外にも普通の文字列を指定することができます。
そして、標準出力をファイル出力する場合、オプションの1番最後に@<code>{> ファイル名}とすると出力することができます。
#@#こういうのなんていうんだろうね、%aみたいなやつ

なので、今回はこのようなgitコマンドを利用して、git logの情報をjsonファイルに出力しました。
@<code>{--pretty=format}の引数で自分の欲しいjsonファイルを整形しておけば、Elasticsearchにデータを簡単に投入することができます。
こうしておけば、LogstashやBeatsを介さなくてもすぐにKibanaをお試しできるってもんです@<fn>{kibana01-fn01}。

//footnote[kibana01-fn01][別にLogstashやBeatsがいらない子とは言っていないぞ！]

ダウンロードzipで落としてきた

//cmd{
~/Elastic-Stack $ ls -al
total 0
drwxr-xr-x   6 mallow  staff   192  3  7 11:00 .
drwxr-xr-x+ 50 mallow  staff  1600  3  7 10:54 ..
drwxr-xr-x@ 11 mallow  staff   352  2 16 19:03 elasticsearch-6.2.2
drwxr-xr-x@ 13 mallow  staff   416  2 17 03:47 filebeat-6.2.2-darwin-x86_64
drwxr-xr-x@ 16 mallow  staff   512  2 17 04:20 kibana-6.2.2-darwin-x86_64
drwxr-xr-x@ 16 mallow  staff   512  3  7 10:51 logstash-6.2.2
//}

Logstashでうまく出力できた時

//cmd{
~/Elastic-Stack/logstash-6.2.2 $ bin/logstash -f config/conf.d/gitlog-logstash.conf
Sending Logstash's logs to /Users/mallow/Elastic-Stack/logstash-6.2.2/logs which is now configured via log4j2.properties
[2018-03-07T13:09:28,580][INFO ][logstash.modules.scaffold] Initializing module {:module_name=>"netflow", :directory=>"/Users/mallow/Elastic-Stack/logstash-6.2.2/modules/netflow/configuration"}
[2018-03-07T13:09:28,609][INFO ][logstash.modules.scaffold] Initializing module {:module_name=>"fb_apache", :directory=>"/Users/mallow/Elastic-Stack/logstash-6.2.2/modules/fb_apache/configuration"}
[2018-03-07T13:09:29,016][WARN ][logstash.config.source.multilocal] Ignoring the 'pipelines.yml' file because modules or command line options are specified
[2018-03-07T13:09:30,030][INFO ][logstash.runner          ] Starting Logstash {"logstash.version"=>"6.2.2"}
[2018-03-07T13:09:30,683][INFO ][logstash.agent           ] Successfully started Logstash API endpoint {:port=>9600}
[2018-03-07T13:09:33,792][INFO ][logstash.pipeline        ] Starting pipeline {:pipeline_id=>"main", "pipeline.workers"=>4, "pipeline.batch.size"=>125, "pipeline.batch.delay"=>50}
[2018-03-07T13:09:34,713][INFO ][logstash.inputs.beats    ] Beats inputs: Starting input listener {:address=>"0.0.0.0:5044"}
[2018-03-07T13:09:34,878][INFO ][logstash.pipeline        ] Pipeline started succesfully {:pipeline_id=>"main", :thread=>"#<Thread:0x28402278 run>"}
[2018-03-07T13:09:35,164][INFO ][org.logstash.beats.Server] Starting server on port: 5044
[2018-03-07T13:09:35,372][INFO ][logstash.agent           ] Pipelines running {:count=>1, :pipelines=>["main"]}
//}

Logstashでデータを取得（詳しい解説は別章をみてやでい）

//cmd{
input {
		file {
			path => "Users/mallow/Google ドライブ/TB4-Elastic-Stack-6/articles/log/*.json"
			tags => "git-log"
		}
}

output {
	stdout { codec => json }
}
//}
