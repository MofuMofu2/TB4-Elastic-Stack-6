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

世の中にはたくさんのログやデータがあります。サーバ運用など、いろんなかたちで携わっている方が多いのではないでしょうか。
例を挙げると、Webサービスのログから分析用途として使用するときや、障害対応でログの調査を行うときに関わりますね。
ログに何らかの形で携わったことがある人は、一度はログやデータの解析は面倒な作業だと思ったのではないでしょうか。@@<fn>{introES-fn03}

//footnote[introES-fn03][micciはあるっていってました。もふもふちゃんは面倒臭そうな作業は苦手ですが、パワーポイント作る方が苦手です。]

ログ収集し、分析するところまでいければまだましな方です。
現実は、まずログ自体の取得が非常に大変で、心が折れてしまうこともあります。
さらに解析対象のサーバが1台ならいいのですが、数十台になってきたら、ツラミでしかないです。そこでLogstashの出番です。
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

Beatsは用途に合わせてデータを簡単に送ることができる軽量データシッパーです@<fn>{introES-fn03}。Go言語で作成されており、動作に必要なリソースが他プロダクトと比較して少ないことが特徴です。
用途に合わせてXXBeatsというように、名前が異なります。ドキュメントも種別ごとに異なりますので、注意が必要です。

//footnote[introES-fn03][https://www.elastic.co/guide/en/beats/libbeat/current/beats-reference.html]

Elastic Stack6からは@<code>{Modules}という機能が追加されました。Modulesを利用すると、Elasticsearchへのデータ連携とKibanaのグラフ作成を自動で行ってくれます。
Apacheのaccess.logなど、利用できるデータは限られています。公式ドキュメント、またはKibanaのGUIで確認してください。

=== Kibana

KibanaはElasticsearch内に保存されているデータを参照し、グラフを利用して可視化できるツールです。Elastic Stack6からは拡張機能を利用することで
Logstashのプロセスの流れをGUIで見える化（@<code>{Logstash pipeline}）することや、Elasticsearchのデータを元に閾値を超えたら通知などのアクションをすることができるようになる
@<code>{Machine Learning}を利用できます。ちなみにLogstash pipelineとMachine Learningは有償機能ですので本同人誌では取り扱いません。同人マンが取り扱うには
5000兆円必要なんだよ！（突然の逆ギレ）

== 今後のElastic Stackはどうなりそう？

この章を書いているもふもふちゃんは2017年12月に開催されたElastic {ON} Tour Tokyoに参加してました。
そこで聞いた情報・また、2018年2月に開催されたカンファレンスElastic {ON}で発表された情報を列挙するぞい！

=== SQLが使えるようになるぞい

Elastic Stackへのクエリを投げるのは今までElasticsearch独自のクエリを書く必要がありました。
しかし、独自のクエリを覚えたり調べたりするのは大変ですよね。

という背景があったのかは謎ですが、今後のアップデートでSQLクエリを利用してElasticsearhにクエリを発行できるようになります。
Insertなど、特定のクエリのみ、かつ標準SQLのサポートになりますが、それでも大分楽になりますね。

=== X-Pack（有償）機能のソースコードが公開されたぞい！

Elastic社からライセンスを購入しないと利用できない機能の名前を@<code>{X-Pack}といいます。今まではソースコードが非公開となっていました。
しかし、Elastic {ON}でX-Packのソースコードが公開されることが発表されました。

@<b>{有償版の機能は引き続きライセンス買わないと使うことはできません}。OSSになったわけではありませんぞ。
インストール時点でわかりますが、ライセンス認証が必要ですからね。ずるしてもわかっちゃうからね。ずるはいけません。

おたくは推しにお布施するじゃないですか？Elastic Stackがいいなーって思うひとはお布施するんだよ！
