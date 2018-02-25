package main

import (
	"context"
	"fmt"

	"github.com/olivere/elastic"
)

func main() {
	esUrl := "http://localhost:9200"
	ctx := context.Background()

	client, err := elastic.NewClient(
		elastic.SetURL(esUrl),
	)
	if err != nil {
		panic(err)
	}

	info, code, err := client.Ping(esUrl).Do(ctx)
	fmt.Printf("Elasticsearch returned with code %d and version %s\n", code, info.Version.Number)

}
