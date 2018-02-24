package main

import (
	"context"

	"github.com/olivere/elastic"
)

func main() {
	ctx := context.Background()
	client, err := elastic.NewClient()
}
