package main

import (
	"context"
	"fmt"
	"reflect"
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

	query := elastic.
	results, err := client.Search().Index("chat").Query(query).Do(ctx)
	if err != nil {
		panic(err)
	}

	var chattype Chat
	for _, chat := range results.Each(reflect.TypeOf(chattype)) {
		if c, ok := chat.(Chat); ok {
			fmt.Println("Chat message is: %s", c.Message)
		}
	}
}
