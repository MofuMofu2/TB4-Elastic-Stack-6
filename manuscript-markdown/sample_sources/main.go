package main

import (
	"context"
	"time"

	"github.com/olivere/elastic"
)

type Chat struct {
	User    string    `json:"user"`
	Message string    `json:"message"`
	Created time.Time `json:"created"`
	Tag     string    `json:"tag"`
}

const (
	ChatIndex = "Chat"
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

	termQuery := elastic.NewTermQuery("user", "山田")
	results, err := client.Scroll("chat").Query(termQuery).Size(10).Do(ctx)
	if err != nil {
		panic(err)
	}

	results, err = client.Scroll("chat").Query(termQuery).Size(10).ScrollId(results.ScrollId).Do(ctx)
	if err != nil {
		panic(err)
	}
}
