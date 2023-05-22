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

const (
	clrSeq = "\033[H\033[2J"
	lowTierGod = `
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
	case v == 1:
		return "A"
	case v >= 2 && v <= 10:
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

type Record struct {
	card      *CardInfo
	startTime time.Time
	task      string
	reps      int
}

type AlmightyThreatener struct {
	deck  []*CardInfo
	tasks []string
	log   []*Record
}

func newDeck() []*CardInfo {
	deck := []*CardInfo{}
	// insert cards to deck
	deck = append(deck, &CardInfo{suit: Joker1, value: 0})
	deck = append(deck, &CardInfo{suit: Joker2, value: 0})
	for s := Spade; s <= Heart; s++ {
		for v := 1; v <= 13; v++ {
			deck = append(deck, &CardInfo{suit: s, value: Value(v)})
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
		log: []*Record{},
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

func readChar(scanner *bufio.Scanner, cue string, chars string) byte {
	for {
		fmt.Print(cue)
		scanner.Scan()
		opt := scanner.Text()[0]
		allowed := []byte(chars)
		if len(allowed) == 0 {
			return opt
		}
		for _, c := range allowed {
			if opt == c {
				return opt
			}
		}
	}
	return '\000'
}

func (t *AlmightyThreatener) getRecord(currentCard *CardInfo) *Record {
	currentTask := t.tasks[currentCard.suit]
	currentReps := int(currentCard.value)
	if currentCard.suit == Spade || currentCard.suit == Club {
		currentReps *= 2
	}
	rec := &Record{
		card:      currentCard,
		startTime: time.Now(),
		task:      currentTask,
		reps:      currentReps,
	}
	t.log = append(t.log, rec)
	return rec
}

func (t *AlmightyThreatener) printLog() {
	fmt.Print(clrSeq)
	fmt.Printf("Cards drawn: %d\n", len(t.log))
	for i, rec := range t.log {
		fmt.Printf("[%d] [%s] %s %s(%d)\n", i+1, rec.startTime.Format(time.ANSIC), rec.card.toString(), rec.task, rec.reps)
	}
}

func (t *AlmightyThreatener) run() {
	scanner := bufio.NewScanner(os.Stdin)
	for i, card := range t.deck {
		rec := t.getRecord(card)
		threaten(rec.card, rec.task, rec.reps)
		if i == len(t.deck)-1 {
			fmt.Println("Deck completed")
			break
		} else if readChar(scanner, "Continue? (y/N) ", "yN") == 'N' {
			break
		}
	}
	if readChar(scanner, "View log? (y/N) ", "yN") == 'y' {
		t.printLog()
	}
}

func threaten(card *CardInfo, task string, reps int) {
	fmt.Print(clrSeq)
	fmt.Printf("%s is drawn", card.toString())
	time.Sleep(2 * time.Second)
	fmt.Print(clrSeq)
	fmt.Printf("You should do %d %s(s)", reps, task)
	fmt.Println(lowTierGod)
	time.Sleep(time.Second)
	fmt.Print(clrSeq)
	fmt.Printf("You should do %d %s(s) NOW!", reps, task)
	fmt.Println(lowTierGod)
}

func main() {
	// todo: constructor with configuration
	t, _ := newAlmightyThreatener()
	t.run()
}
