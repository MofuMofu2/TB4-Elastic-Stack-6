# Multiple Pipelinesの世界へようこそ

## 複数のログを取り扱う場合

ここまで一つのログを対象にしてきましたが、実際は多数のログを対象にすることが多いと思います。  
多数のログを取り扱う場合は、conf.d配下にパイプラインの設定ファイルを配置します。  
例えば、ALBのログとhttpdログの場合は、以下になります。

```bash
### Logstash directory
/etc/logstash/
 ┣ conf.d
 ┃ ┣ alb.conf
 ┃ ┗ httpd.conf
 ┣ jvm.options
 ┣ log4j2.properties
 ┣ logstash.yml
 ┣ pipelines.yml
 ┗ startup.options
```

このようにconf.dに配置したファイルが対象の設定ファイルと見なされます。
ログによっては、ログ量やフィルタなど負荷のかかり具合が違ってくるのですが、柔軟なリソース供給ができません。
前章で説明したWoker数などを設定ファイル毎に設定することができないのです。

しかし、Logstashの6系から柔軟なリソース設定が可能になりました。
それでは、Multiple Pipelinesの説明をしていきたいと思います。


