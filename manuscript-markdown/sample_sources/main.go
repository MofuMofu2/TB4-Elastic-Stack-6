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

	chatData := Chat{
		User:    "user01",
		Message: "test message",
		Created: time.Now(),
		Tag:     "tag01",
	}

	_, err = client.Index().Index("chat").Type("chat").Id("1").BodyJson(&chatData).Do(ctx)
	if err != nil {
		panic(err)
	}

}
