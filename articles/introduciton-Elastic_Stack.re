= Elastic Stackとは

Elastic Stackは、Elasticsearch社が提供するプロダクトです。
2016年までは、Elasticsearch、Logstash、Kibanaの頭文字をとったELKという呼び名で親しまれていました。
しかし、Beatsという新たなプロダクトが増えたことにより、ELKでは違和感があるのと、ELKにうまい具合にB（Beatsの頭文字）を追加することも難しくなりました。
そこでELKではなく「Elastic Stack」という呼び方に統合し、主に次の4つがプロダクトとして構成されています。

 * Elasticsearch
 * Logstash
 * Beats
 * Kibana

この4つは有名かつOSSとして利用できるプロダクトです。特に検索エンジンとしてのElasticsearchは競合がいないのでは？というくらい
よく使われているミドルウェアです。

Elasticsearchのプロセス監視に特化したWatcher、Elasticsearchに保存されているデータの傾向を観察し
異常なデータがあればアラートをあげるMachine LearningなどもElastic Stackの中に含まれています。しかし、これらのプロダクトは有償利用となるため
この本では扱いません。

Elastic Stackはやりたいことを実現できるだけのカスタマイズ性の高さ、より利便性を求めてアップデートをかけていく姿勢が魅力です。

== 主要プロダクトの紹介

Elasticsearh・Logstash・Kibanaの記事を読む際は、バージョンが5より前か後かをきちんと確認してください。コンセプトも少しずつ変化していますし、何より機能の統廃合が進みすぎているので
昔「ELK」と呼ばれていたものと現在のElastic Stackはもはや別物です。

=== Elasticsearch

@<code>{Elastic Stackで作るBI環境 誰でもできるデータ分析入門（石井 葵著、インプレスR&D刊） }（@<href>{https://nextpublishing.jp/book/8889.html}）によると

//quote{
Elasticsearchは、Javaで作られている分散処理型の検索エンジンです。クラスタ構成を組むことができるのが特徴なので、大規模な環境で検索エンジンとして利用されることがあります。
//}

と説明されています。
用途としては、リアルタイムデータ分析、ログ解析、全文検索などさまざまなところで利用されています。

昔は自分たちでElasticsearchを構築・運用するか、AWSの機能として@<code>{Amazon Elasticsearch Service}（@<href>{https://aws.amazon.com/jp/elasticsearch-service/}）を利用するしかありませんでした。
しかし、Elastic Stack5からはElasticsearch社が提供するクラウドサービス@<code>{Elastic Cloud}（@<href>{https://www.elastic.co/jp/cloud}）を利用することで、Elasticsearchの管理・バージョンアップ・
データのバックアップなども柔軟に行うことができるようになりました。基盤の持ち方の選択肢が増えるのはありがたいですよね。

Elasticsearchは独自のクエリを使用してデータの問い合わせをおこなうことが特徴です。が、今後のアップデートでSQLを利用してデータの問い合わせをできるようになることが発表されています（@<href>{https://www.elastic.co/jp/elasticon/conf/2017/sf/elasticsearch-sql}）。
SQLの方が普及率も高いので、さらにElasticsearchを便利に利用することができそうです。

=== Logstash

世の中にはたくさんのログやデータがあります。サーバ運用など、いろんなかたちで携わっている方が多いのではないでしょうか。
例を挙げると、Webサービスをログから分析したり、障害対応でログの調査を行うときが考えられますね。
ログに何らかの形で携わったことがある人は、一度はログやデータの解析は面倒な作業だと思ったのではないでしょうか。@<fn>{introES-fn03}

//footnote[introES-fn03][本章の筆者は面倒臭そうな作業は苦手ですが、パワーポイント作る方がもっと苦手です。]

ログ収集し、分析するところまでいければまだましな方です。
現実は、まずログ自体の取得が非常に大変で、心が折れてしまうこともあります。
さらに解析対象のサーバが1台ならいいのですが、数十台では辛さしかありません。そこでLogstashの出番です。
Logstashは各環境に散らばっているログを集め、指定した対象に連携できるツールです。ログの連携だけではなく、ログの加工機能も持ち合わせています@<fn>{introES-fn02}。

//footnote[introES-fn02][Elastic Stackで作るBI環境 誰でもできるデータ分析入門（石井 葵著、インプレスR&D刊）}（@<href>{https://nextpublishing.jp/book/8889.html}）]

ログの取得というとファイルからの取得を思い浮かべますが、プラグインを利用することでAmazon s3やTwitterから直接データを取得することも可能です。

類似プロダクトとしてはTresure Data社製のOSSである、@<code>{fluentd}（@<href>{https://www.fluentd.org}）が存在します。
エラーのわかりやすさ、環境構築の簡単さを取るのであればfluentdを、Elastic Stackとしてプロダクトをセットで運用するのであればLogstashを利用するとよいでしょう。

Logstashはバージョン6からLogstashのプロセスを@<code>{Multiple pipeline}として分割できるようになりました。これを利用すると、AのデータとBのデータをLogstashで取得したいときに
Logstashプロセスを2つ作ることができます。片方のプロセスが止まっても、もう片方のデータ連係は継続して行うことができるので、対障害性が上がります。
詳しくは「AWSでLogstashを使ってみる」を参照してください。

=== Beats

Beatsは用途に合わせてデータを簡単に送ることができる軽量データシッパーです@<fn>{introES-fn04}。Go言語で作成されており、動作に必要なリソースが他プロダクトと比較して少ないことが特徴です。
用途に合わせてXXBeatsというように、名前が異なります。ドキュメントも種別ごとに異なりますので、注意が必要です。

//footnote[introES-fn04][https://www.elastic.co/guide/en/beats/libbeat/current/beats-reference.html]

Elastic Stack6からは@<code>{Modules}という機能が追加されました。Modulesを利用すると、Elasticsearchへのデータ連係とKibanaのグラフ作成を自動で行ってくれます。
ただし、Apacheのaccess.logなど利用できるデータが限られています。公式ドキュメント、またはKibanaのGUIで確認してください。

==== Filebeat


サーバの大量のログファイルなどのファイルを一箇所に集約する用途で用います。
また、集約だけでなく、転送時にあらかじめ用意されたモジュールを利用することで、自動でパース処理を行い、ElasticsearchやLogstashに転送することが可能です。
さらに取り込んだデータをビジュアライズするためのダッシュボードも用意されており、簡単に導入することができます。


==== Metricbeat


メトリックという名前だけあって、システムやサービスのメトリックを収集することができます。
たとえば、サーバのCPUや、メモリの使用率、ディスクIOなどのデータだけでなく、プロセスなども収集できます。
また、ビジュアライズするためのダッシュボードもあらかじめ用意されており、こちらも導入が簡単です。



サービスについても簡単に収集するためのモジュールが用意されており、PostgreSQLやDockerなどのメトリクスを取得可能です。


==== Packetbeat


ネットワークを流れるパケットを収集することができます。
パケットを収集するためにWiresharkなどで取得する場面があると思いますが、Packetbeatはより簡単に専門的な知識がなくてもビジュアライズまで可能とするものです。
さまざまなプロトコルに対応しているため、MySQLのクエリなどをKibanaを用いてビジュアライズすることも可能です。


==== Winlogbeat


Windowsのイベントログを収集することができます。
たとえば、Windowsサーバを運用しており、監査目的でログオンしたユーザを把握したい場合は、イベントIDの"ログオン: 4624"や"ログオン失敗: 4625"を指定するだけでイベントログを収集することが可能です。
取得したいイベントIDを指定するだけなので、簡単に導入できます。

Windowsの動作ログを取得したい場合、1番導入が簡単で手軽なプロダクトなのではないでしょうか。


==== Auditbeat


サーバの監査ログを収集することができます。
通常、auditdのログを監査ログとして利用する場面が多いと思いますが、Auditbeatを使用することで、必要な情報をグルーピングし、Elasticsearchに転送することができます（要は意識せずストアまでやってくれます）。
また、Modulesに対応しているため、導入からKibanaを用いたデータの可視化までを一括で行うことができます。


==== Heartbeat

サーバの稼働状況を監視できます。
ICMPでサーバの稼働状況を把握することも可能ですし、HTTPでサービス稼働も把握することが可能です。
また、TLS、認証やプロキシにも対応しているため、あらゆる状況でも稼働状況を監視することができます。

=== Kibana

KibanaはElasticsearch内に保存されているデータを参照し、グラフを利用して可視化できるツールです。Elastic Stack6からは拡張機能を利用することで
Logstashのプロセスの流れをGUIで可視化（@<code>{Logstash pipeline}）することや、Elasticsearchのデータを元に閾値を超えたら通知などのアクションをすることができるようになる
@<code>{Machine Learning}を利用できます。なお、Logstash pipelineとMachine Learningは有償になっています。

== 今後のElastic Stack

2017年12月に開催されたElastic {ON} Tour Tokyoで発表された情報と、
2018年2月に開催されたカンファレンスElastic {ON}で発表された情報から、注目度が高いものを記述します。

=== Elaticsearchへクエリを投げる際、SQLが利用可能に

Elastic Stackへのクエリを投げるために、今までElasticsearch独自のクエリを書く必要がありました。
しかし、独自のクエリを覚えたり調べたりするのは大変ですよね。

今後のアップデートで、SQLクエリを利用してElasticsearhにクエリを発行できるようになります。
Insertなど特定のクエリのみ、かつ標準SQLのサポートになりますが、それでも大分楽になりますね。

=== X-Pack（有償）機能のソースコードを公開

Elasticsearch社からライセンスを購入しないと利用できない機能の名前を@<code>{X-Pack}といいます。今まではソースコードが非公開となっていました。
しかし、Elastic {ON}でX-Packのソースコードが公開されることが発表されました。

@<b>{有償版の機能は引き続きライセンスを買わないと使うことはできません}。OSSになったわけではありません。
