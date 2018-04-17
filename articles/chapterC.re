= fluentdとLogstashをはじめるまえの準備
コンフィグや動作の比較をするのであれば、実際に動かしながらの方がわかりやすいですよね。
というわけで、事前準備としてインストール方法やプラグインの探し方などを紹介します。
今回はCentOS6上で動作検証を行っていますので、別のOSを利用する場合や詳しく知りたい場合は
公式ドキュメントや技術書を参照してください@<fn>{mofumofu_tweet}。

今回、動作環境として使用したサーバーはテコラス社のCloudGarageです@<href>{https://cloudgarage.jp}。
2017年の夏からサービスを開始したできたてほやほやの国産クラウドサービスです。
1ヶ月でサーバーがまとめて借りられることと、一定のお値段であることがお財布に優しいですね@<fn>{cloud_services}。
また、今回はfluentdとLogstashを別々のサーバーにインストールしています。

//footnote[cloud_services][ちょこっとミドルウェアの動作確認するのに、AWSでネットワーク構築したりするのはめんどかったというのは内緒なのです。]

//footnote[mofumofu_tweet][もふもふちゃんの同人誌を買ってもらってもいいんですよー（ただしLogstashに限る）]

== インストール
=== fluentd

==== fluentdのインストール
fluentdは@<code>{td-agent}という名称で公式からパッケージが配布されています。インストールする際はこちらを利用することが
推奨されています。ありがたく使わせていただきましょう。提供されているシェルスクリプトをcurlコマンドでGETすると
インストールまで完了します。スクリプトの名前はOSごとに違いますので、公式サイトを確認してください。

//list[fluentd_install][fluentdのインストール]{
curl -L https://toolbelt.treasuredata.com/sh/install-redhat-td-agent2.sh | sh
//}

スクリプトを実行すると、次のように出力されます。

//cmd{
# curl -L https://toolbelt.treasuredata.com/sh/install-redhat-td-agent2.sh | sh
  % Total    % Received % Xferd  Average Speed   Time    Time     Time  Current
                                 Dload  Upload   Total   Spent    Left  Speed
100  1109  100  1109    0     0   4515      0 --:--:-- --:--:-- --:--:--  541k
==============================
 td-agent Installation Script
==============================
This script requires superuser access to install rpm packages.
# 記載省略
完了しました!

Intallation completed. Happy Logging!

NOTE: In case you need any of these:
  1) security tested binary with a clear life cycle management
  2) advanced monitoring and management
  3) support SLA
Please check Fluentd Enterprise (https://fluentd.treasuredata.com/).
//}

私たちもHappy Loggingしたいですね。


==== 動作確認
インストール後は@<code>{/etc/init.d/td-agent}にデーモンスクリプトが配置されますので、起動できるかを確認しましょう。

//list[fluentd_start][fluentdの起動|停止|サービス状態の確認|リスタート]{
service td-agent start|stop|status|restart
//}

実際にコマンドを発行すると、次のように出力されます。
//cmd{
# 起動前のステータス確認
$ service td-agent status
td-agent is not running                                    [FAILED]

# fluentdの起動
$ service td-agent start
td-agent td-agent:                                         [  OK  ]

# fluentdの起動後
$ service td-agent status
td-agent is running                                        [  OK  ]
//}


fluentdの設定ファイルは@<code>{/etc/td-agent/}配下にありますが、
実体は@<code>{/opt/td-agent/}に配置されています。


//list[fluentd_etc][/etc/td-agentの中身]{
drwxr-xr-x   3 root root 4096 10月  4 19:18 2017 .
drwxr-xr-x. 70 root root 4096 10月  4 19:18 2017 ..
drwxr-xr-x   2 root root 4096 10月  4 19:18 2017 plugin
-rw-r--r--   1 root root 2081 10月  4 19:18 2017 td-agent.conf
//}

//list[fluentd_opt][/opt/td-agentの中身]{
drwxrwxr-x  7 root root  4096 10月  4 19:18 2017 .
drwxr-xr-x. 4 root root  4096 10月  4 19:18 2017 ..
-rw-rw-r--  1 root root 15337  4月 26 03:49 2017 LICENSE
drwxrwxr-x  2 root root  4096 10月  4 19:18 2017 LICENSES
drwxrwxr-x  2 root root  4096  4月 26 03:49 2017 bin
drwxrwxr-x  7 root root  4096 10月  4 19:18 2017 embedded
drwxrwxr-x  4 root root  4096 10月  4 19:18 2017 etc
drwxrwxr-x  4 root root  4096 10月  4 19:18 2017 usr
-rw-rw-r--  1 root root  8928  4月 26 03:49 2017 version-manifest.json
-rw-rw-r--  1 root root  3943  4月 26 03:49 2017 version-manifest.txt
//}


最後に、テストデータをfluentdに投げて処理ができるかも確認しておきましょう。ポートが空いているかは事前に確認しておくと良いです。

//list[fluentd_test][fluentdの動作テスト]{
curl -X POST -d 'json={"json":"message"}' http://localhost:8888/debug.test
//}

@<code>{/var/log/td-agent}配下にある@<code>{td-agent.log}に次のような出力があれば、
fluentdは正常に動作しています。

//list[fluetd_testlog][td-agent.logの出力結果（抜粋）]{
2017-10-04 19:28:58 +0900 debug.test: {"json":"message"}
//}

これでfluentdの準備は完了です。


=== Logstash

==== Javaのインストール
LogstashはJRubyでプラグインが作成されているため、動作にはJavaが必要です。
バージョン8以上が動作要件となりますが、こだわりがなければJava8の最新版をインストールすれば良いでしょう。

//cmd{
# Javaがインストールされていない場合
$ java -version
-bash: java: コマンドが見つかりません

# インストール
$ sudo yum install java-1.8.0-openjdk-devel

# インストール後
$ java -version
openjdk version "1.8.0_141"
OpenJDK Runtime Environment (codeuild 1.8.0_141-b16)
OpenJDK 64-codeit Server VM (codeuild 25.141-b16, mixed mode)
//}

=== Logstashのインストール
Logstashも公式からインストール用パッケージが配布されていますので、ご自分の環境に合わせてパッケージを持ってきましょう@<fn>{Logstash_pack}。
今回記載する例はCentOS6にインストールする場合のみなので、UbuntuやWindows派の方は公式ドキュメントを参考にしてください。
まずは@<code>{/etc/yum.repos.d}配下に@<code>{logstash.repo}を作成し、次の内容を記載します。

//list[Logstash_repo][logstash.repo]{
[logstash-6.x]
name=Elastic repository for 6.x packages
codeaseurl=https://artifacts.elastic.co/packages/6.x-prerelease/yum
gpgcheck=1
gpgkey=https://artifacts.elastic.co/GPG-KEY-elasticsearch
enabled=1
autorefresh=1
type=rpm-md
//}

リポジトリ登録後、yumコマンドでインストールします。

//list[Logstash_install][yumによるLogstashのインストール]{
yum install logstash
//}

//footnote[Logstash_pack][ただし2017年10月時点ではバージョン6はbeta版なので、本番環境に導入するのはやめといた方がいいです。バージョン5は本番環境でも使えます。]


==== 動作確認用のlogstash.conf作成
次に、動作確認用のコンフィグを記載します。@<code>{/etc/logstash}配下にlogstash.confを作成し、次の内容を転記してください。

//list[Logstash_test_config][動作確認用のlogstash.conf]{
input {
	stdin { }
}

output {
	stdout { }
}
//}

=== Logstashの動作確認
Logstashの起動方法は2通りあります。

 1. @<code>{/usr/share/logstash/bin}下にある@<code>{logstash}スクリプトから、Logstashを起動する。
 2. サービスコマンドを使用する。

1の方法でLogstashの起動を行う場合、次の通りに実行します。
@<code>{-f}オプションで@<code>{logstash.conf}を指定してください。


//list[Logstash_bin_start][Logstashの起動その1]{
bin/logstash -f /etc/logstash/conf.d/logstash.conf
//}

Logstash起動後、標準入力から好きな文字を入力してみます。入力結果がそのまま返ってくればインストールできています。

//cmd{
$ bin/logstash -f /etc/logstash/conf.d/logstash.conf
The stdin plugin is now waiting for input:
test ←入力したもの
2017-09-27T08:53:59.456Z 0.0.0.0 test ←出力結果
//}


ちなみに2の方法でLogstashの起動を行う場合、次の通りに実行します。
ただし、OSによってサービス起動コマンドは異なります@<fn>{Logstash_start_command}。

//footnote[Logstash_start_command][参考：https://www.elastic.co/guide/en/logstash/6.0/running-logstash.html]


//list[Logstash_service_start][Logstashの起動その2（Cent0S6の場合）]{
initctl start logstash
//}

== プラグインの探しかた
fluentdとLogstashはプラグイン形式で機能が提供されています。インストール時点でも様々なデータを処理できるように
なっていますが、足りない機能は別の人が作成したプラグインで補完することもできます。@<href>{https://rubygems.org/gems}で
探すと良いですが、本番環境への投入を考えるのであればよく動作検証した方が良いです。
もちろん、RubyやJRubyが書けるのであればプラグインを自作することもできます。こういう点は、OSSの良いところですね。

=== fluentd
プラグインは@<href>{https://www.fluentd.org/plugins}に一覧が記載されています。
カテゴリごとに見ることができるので、用途に応じたプラグインを探しやすくなっています。こういったところに優しさが感じられますね。
各プラグイン名のURLリンクをクリックすると、GithubのREADMEページへジャンプします。コンフィグの書き方や使用方法を確認して
使っていきましょう。


=== Logstash
公式で提供されているプラグインはドキュメントに記載がありますが、こちらはinput、filter、outputごとにページが別れています。
@<href>{https://www.elastic.co/guide/en/logstash/6.x/index.html}を見ながらプラグインを探しましょう。ちなみに
LogstashのバージョンごとにURLが異なりますので注意してください@<fn>{logstash_documents}。

//footnote[logstash_documents][バージョンアップによってプラグインのオプションで提供される機能が変わったり、プラグインごと消えてたりしますので。]
