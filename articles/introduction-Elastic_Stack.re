= Elastic Stackとは

Elastic StackとはElastic社が開発しているプロダクト群です。

* Elasticserch
* Logstash
* Beats
* Kibana

この辺りが有名どころ、かつOSSとして利用できるプロダクトですね。特に検索エンジンとしてのElasticsearchは競合がいないのでは？というくらい
よく使われているミドルウェアです。

やりたいことに合わせて柔軟に答えていけるだけのカスタマイズ性の高さ、より利便性を求めてアップデートをかけていく姿勢が魅力です。
まるでメタモンですね。メタモンはアップデートかからないけどね。



== 主要プロダクト

Elastic Stackが今の形になったのは2016年です。Elastic Stack5までは各プロダクトのバージョンはバラバラに管理されており、リリース時期も
プロダクトごとに異なっていました。さらに、5以前はElasticsearch・Logstash・Kibanaしかプロダクトは存在しませんでした。

これら3つのプロダクトの記事を読む際は、バージョンが5より前か後かをきちんと確認した方が良いです。コンセプトも少しずつ変化していますし、何より機能の統廃合が進みすぎているので
昔「ELK」と呼ばれていたものとElastic Stackはもはや別物です。

=== Elasticsearch

@<code>{Elastic Stackで作るBI環境 誰でもできるデータ分析入門 (技術書典シリーズ(NextPublishing)) }（@<href>{https://www.amazon.co.jp/Elastic-Stackで作るBI環境-誰でもできるデータ分析入門-技術書典シリーズ-NextPublishing/dp/484439780X}）によると

//quote{
Elasticsearchは、Javaで作られている分散処理型の検索エンジンです。クラスタ構成を組むことができるのが特徴なので、大規模な環境で検索エンジンとして利用されることがあります。
//}

と説明されています@<fn>{introES-fn01}。

//footnote[introES-fn01][技術書典シリーズって名前で察した方も多いかと思いますが、これは技術書典2でもふもふちゃんがElastic Stack5の同人誌を書いたらインプレスR&Dさんが商業本にしてくれました。KindleでElasticて打つと検索トップに出てくるからびびるんだけど。]

昔は自分たちでElasticsearchを構築・運用するか、AWSの機能として@<code>{Amazon Elasticserch Service}（@<href>{https://aws.amazon.com/jp/elasticsearch-service/}）を利用するしかありませんでした。
しかし、Elastic Stack5からはElastic社が提供するクラウドサービス@<code>{Elastic Cloud}（@<href>{https://www.elastic.co/jp/cloud}）を利用することでElasticsearchの管理・バージョンアップ・
データのバックアップなども柔軟に行うことができるようになりました。基盤の持ち方の選択肢が増えるのはありがたいですよね。

Elasticserchは独自のクエリを使用してデータの問い合わせをおこなうことが特徴です。が、今後のアップデートでSQLを利用してデータの問い合わせをできるようになることが発表されています@<href>{https://www.elastic.co/jp/elasticon/conf/2017/sf/elasticsearch-sql}。
SQLの方が普及率も高いので、さらにElasticsearchを便利に利用することができそうです。

=== Logstash

Logstashは各環境に散らばっているログを集め、指定した対象に連携できるツールです。ログの連携だけではなく、ログの加工機能も持ち合わせています@<fn>{introES-fn02}。

//footnote[introES-fn02][Elastic Stackで作るBI環境 誰でもできるデータ分析入門 (技術書典シリーズ(NextPublishing)) }（@<href>{https://www.amazon.co.jp/Elastic-Stackで作るBI環境-誰でもできるデータ分析入門-技術書典シリーズ-NextPublishing/dp/484439780X}）]

ログの取得というとファイルからの取得を思い浮かべますが、プラグインを利用することでAmazon s3やTwitterから直接データを取得することも可能です。

類似プロダクトとしてはTresure Data社製のOSS@<code>{fluentd}（URL:@<href>{https://www.fluentd.org}）が存在します。
エラーのわかりやすさ、環境構築の簡単さを取るのであればfluentdを、Elastic Stackとしてプロダクトをセットで運用するのであればLogstashを利用すると良いでしょう。

Logstashはバージョン6からLogstashのプロセスを@<code>{Multiple pipeline}として分割できるようになりました。これを利用すると、AのデータとBのデータをLogstashで取得したいときに
Logstashプロセスを2つ作ることができます。片方のプロセスがお亡くなりになってももう片方のデータ連携は継続して行うことができるので、対障害性が上がりますね。
詳しくはXXを参照してください。

#@# todo 参照するぞ

=== Beats

Beatsは用途に合わせてデータを簡単に送ることができる軽量データシッパーです。Go言語で作成されており、動作に必要なリソースが他プロダクトと比較して少ないことが特徴です。
用途に合わせてXXBeatsというように、名前が異なります。ドキュメントを参照するときは注意したようが良いです。

#@#HPのURLリンク貼る

Elastic Stack6からは@<code>{Modules}という機能が追加されました。Modulesを利用すると、Elasticsearchへのデータ連携とKibanaのグラフ作成を自動で行ってくれます。
Apacheのaccess.logなど、利用できるデータは限られています。公式ドキュメント、またはKibanaのGUIで確認してください。

=== Kibana

KibanaはElasticsearch内に保存されているデータを参照し、グラフを利用して可視化できるツールです。Elastic Stack6からは拡張機能を利用することで
Logstashのプロセスの流れをGUIで見える化（@<code>{Logstash pipeline}）することや、

== 今後のElastic Stack6のみどころだよ

有償版のプロダクトやSaaS、クラウドに関する情報はここから除外します。だって使えないやんけ！同人誌マンには使いどころがないんだよ！（突然の逆ギレ）


Elastic Stackへのクエリを投げるのは今までElasticsearch独自のクエリを書く必要がありました。
しかし、独自のクエリを覚えたり調べたりするのは大変ですよね。

と、いう背景があったのかは謎ですが、今後のアップデートでSQLクエリを利用してElasticsearhにクエリを発行できるようになります。
Insertなど、特定のクエリのみ、かつ標準SQLのサポートになりますが、それでも大分楽になりますね。

あ、そうそう、X-Packはソースコードが公開となりました。が、@<b>{有償版の範囲は引き続きライセンス買わないと使うことはできません}。
インストール時点でわかりますが、ライセンス認証が必要ですからね。ずるしてもわかっちゃうからね。ずるはいけません。

おたくは推しにお布施するじゃないですか？Elastic Stackがいいなーって思うひとはお布施するんだよ！
