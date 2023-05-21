/**
 * A tiny program that threatens you to take the Gotch bible, a card-deck workout method by a
 * legendary wrestler named Karl Gotch. Refer to https://andreian.com/gotch-bible/ for details
 *
 * This program simulates the task of shuffling deck, and hands out exercise in the format of
 * "You should do [exercise], NOW" along with a braille art of LowTierGod (for the lel).
 *
 * As this is just a toy program to learn Go, there might be some additions and modifications
 * later on, idk.
 */

package main

import (
	"bufio"
	"fmt"
	"math/rand"
	"os"
	"time"
)

type Suit int

const (
	Spade Suit = iota
	Club
	Diamond
	Heart
	Joker1 // joker cards
	Joker2
)

var suitName = []string{
	"♠",
	"♣",
	"♦",
	"♥",
	"Joker 1",
	"Joker 2",
}

func (s Suit) toString() string {
	if s >= Spade && s <= Joker2 {
		return suitName[s]
	}
	return fmt.Sprintf("Card suit(%d)?", int(s))
}

type Value int

func (v Value) toString() string {
	switch {
	case v >= 1 && v <= 10:
		return fmt.Sprintf("%d", int(v))
	case v == 11:
		return "J"
	case v == 12:
		return "Q"
	case v == 13:
		return "K"
	default:
		return fmt.Sprintf("Card value(%d)?", int(v))
	}
}

type CardInfo struct {
	suit  Suit
	value Value
}

func (i *CardInfo) toString() string {
	s := i.suit.toString()
	if i.suit != Joker1 && i.suit != Joker2 {
		s += " " + i.value.toString()
	}
	return s
}

type LogInfo struct {
	card      CardInfo
	startTime time.Time
	task      string
	number    int
}

type AlmightyThreatener struct {
	deck  []CardInfo
	tasks []string
	logs  []LogInfo
}

func newDeck() []CardInfo {
	deck := []CardInfo{}
	// insert cards to deck
	deck = append(deck, CardInfo{suit: Joker1, value: 0})
	deck = append(deck, CardInfo{suit: Joker2, value: 0})
	for s := Spade; s <= Heart; s++ {
		for v := 1; v <= 13; v++ {
			deck = append(deck, CardInfo{ suit: s, value: Value(v) })
		}
	}
	// shuffle deck
	rand.Seed(time.Now().UnixNano())
	rand.Shuffle(len(deck), func(i, j int) { deck[i], deck[j] = deck[j], deck[i] })
	return deck
}

func newAlmightyThreatener() (*AlmightyThreatener, error) {
	res := &AlmightyThreatener{
		deck: newDeck(),
		tasks: []string{
			Spade:   "hindu squat",
			Club:    "jump squat",
			Diamond: "hindu pushup",
			Heart:   "half-moon pushup",
			Joker1:  "hindu squat",
			Joker2:  "half-moon pushup",
		},
		logs: []LogInfo{},
	}
	return res, nil
}

// func (t *AlmightyThreatener) dummyPrint() {
// 	fmt.Printf("Size of deck: %d\n", len(t.deck))
// 	for _, x := range t.deck {
// 		fmt.Printf("It's a %s. ", x.toString())
// 		fmt.Printf("Do %d %s(s)\n", x.value, t.tasks[x.suit])
// 	}
// }

func readYNFromStdin(scanner *bufio.Scanner) byte {
	for {
		fmt.Printf("Continue? (y/N) ")
		scanner.Scan()
		c := scanner.Text()[0]
		if c == 'y' || c == 'N' {
			return c
		}
	}
	return '\000'
}

func (t *AlmightyThreatener) run() error {
	scanner := bufio.NewScanner(os.Stdin)

	for _, x := range t.deck {
		move := t.tasks[x.suit]
		reps := int(x.value)
		if x.suit == Spade || x.suit == Club {
			reps *= 2
		}
		threaten(&x, move, reps)
		// todo: logs

		if readYNFromStdin(scanner) == 'N' {
			break
		}
	}
	return nil
}

func threaten(card *CardInfo, task string, reps int) {
	lowTierGod := `
⠀⠀⢵⠁⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢹⠀⠀
⠀⡀⠈⢧⠠⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⡎⠀⠀
⠀⠀⠠⣸⡆⠆⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⡀⠀⠀⠸⠁⠀⠀
⠀⠀⠀⠈⢿⠀⠀⠀⠀⣀⣀⣤⣤⣤⣤⣤⣄⣀⣀⡠⢠⢴⠶⠞⠀⠀⠀⠀⡇⠀⠀
⠀⠀⠀⠀⢐⣿⠀⣼⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣷⠆⠀⠀⠀⠀⣠⡾⠇⠀⠀
⠀⠀⠂⠀⣰⡏⣸⡇⢹⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿⢇⠀⠀⢀⣾⠏⠀⠀⠀⠀
⢈⠁⠀⢠⣿⡧⣿⠂⢈⣿⣟⠻⢿⣿⣿⠿⠛⣛⣛⢛⣯⡌⠀⠀⣿⡇⠀⠀⠀⠀⠀
⠀⣔⢵⣾⣿⢉⣿⣃⣴⣿⣿⣧⢸⣿⣿⣧⣶⣾⣿⣿⣿⣷⣤⣲⣿⠑⠄⠀⠀⠀⠀
⡀⢽⣿⣿⣿⠄⣿⣿⣿⣿⣿⠟⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣟⣻⡇⠀⠈⠂⠀⠀⠀
⣿⡀⢀⣿⣿⡆⣿⠻⣿⣿⡿⠚⢿⡿⠿⣿⣿⣿⣿⣿⣿⣿⢿⣿⠃⠀⠀⠈⠢⢀⠀
⣿⣿⣿⣿⣿⣷⢿⣷⠻⣿⣿⡶⢰⡿⣿⣾⣿⣿⣿⣿⣿⣿⠘⠋⢦⠀⠀⠀⠀⠸⡀
⣿⣿⣿⣿⣿⣿⡼⣿⣷⠨⠒⠚⠛⠻⠷⠿⠿⣻⣿⣿⣿⠏⠀⠀⢀⣧⠀⠀⢀⡀⡴
⣿⣿⣿⣿⣿⣿⣿⡙⣿⠀⠛⠿⠿⠿⢿⣿⣿⣿⣿⡟⢁⡄⠀⠀⡘⠿⣷⣄⡸⣐⡅
⣿⣿⣿⣿⣿⣿⣿⣿⣌⢇⠀⢴⣶⣾⣿⣿⣿⠟⠉⣀⣾⣷⠀⠐⠀⠀⠘⢿⣿⣿⣇
⣿⣿⣿⣿⣿⠟⠉⢙⣿⠄⠀⠀⠉⠛⠉⠉⠀⠀⣶⣿⣿⣿⣽⣿⣷⣤⠀⠘⠿⣿⣿
⣿⣿⣿⣿⠏⠀⠀⠀⠁⠀⠆⠀⠀⠀⠀⠀⢀⣿⣿⣿⣿⣿⣿⣿⠟⠃⠀⠀⠈⠺⠛
⠉⠙⠙⠁⠀⠀⠀⠀⠀⠀⠸⡄⠀⢤⣶⣶⣿⣿⣿⣿⣿⠿⠋⠁⠀⠀⠀⠀⠀⠀⠀
⠄⠀⠀⠀⠀⠀⢀⡀⠤⠔⠒⠈⠳⡦⣿⣿⡿⠿⠛⠉⠁⠀⠀⠀⠀⠀⠀⣀⠄⠂⠙`
	fmt.Printf("\033[H\033[2J%s is drawn", card.toString())
	time.Sleep(3 * time.Second)
	fmt.Printf("\033[H\033[2JYou should do %d %s(s)", reps, task)
	fmt.Println(lowTierGod)
	time.Sleep(time.Second)
	fmt.Printf("\033[H\033[2JYou should do %d %s(s) NOW!", reps, task)
	fmt.Println(lowTierGod)
}

func main() {
	// todo: constructor with configuration
	t, _ := newAlmightyThreatener()
	t.run()
}