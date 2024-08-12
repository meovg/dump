package main

import (
    "encoding/csv"
    "fmt"
    "os"
    "strings"
)

func main() {
    // Input data
    input := `기생충 [Parasite] (2019)
    5.0     
🇰🇷  
브로커 [Broker] (2022)
    3.5     
🇰🇷  
아가씨 [The Handmaiden] (2016)
    4.5     
🇰🇷  
올드보이 [Oldboy] (2003)
    5.0     
🇰🇷  
아티스트 봉만대 [Playboy Bong] (2013)
    0.5     
🇰🇷  
콘크리트 유토피아 [Concrete Utopia] (2023)
    2.5     
🇰🇷  
살인의 추억 [Memories of Murder] (2003)
    4.5     
🇰🇷  
헤어질 결심 [Decision to Leave] (2022)
    4.0     
🇰🇷  🍿  
00:08 (2014)
    3.5     
🇯🇵  short  
101 Dalmatians (1996)
    2.0     
🇺🇸  
21 Jump Street (2012)
    3.0     
🇺🇸  
Aftersun (2022)
    4.5     
🇬🇧  🇺🇸  
All of Us Strangers (2023)
    3.0     
🇬🇧  
American Beauty (1999)
    4.0     
🇺🇸  
Anatomie d'une chute [Anatomy of a Fall] (2023)
    3.5     
🇫🇷  
Andy Warhol Eats a Hamburger (1982)
    3.0     
🇺🇸  short  
Ant-Man (2015)
    3.0     
🇺🇸  capeshit  
The Avengers (2012)
    3.0     
🇺🇸  capeshit  
Avengers: Age of Ultron (2015)
    2.5     
🇺🇸  capeshit  
Avengers: Infinity War (2018)
    3.0     
🇺🇸  capeshit  
Baby Driver (2017)
    3.5     
🇬🇧  🇺🇸  
Bao giờ cho đến tháng mười [When the Tenth Month Comes] (1984)
    3.5     
🇻🇳  
Bean (1997)
    3.5     
🇬🇧  🇺🇸  
Bee Movie (2007)
    3.0     
🇺🇸  
Big Hero 6 (2014)
    3.5     
🇺🇸  capeshit  
The Big Lebowski (1998)
    4.5     
🇺🇸  
Birdman or (The Unexpected Virtue of Ignorance) (2014)
    4.0     
🇺🇸  
Blade Runner 2049 (2017)
    4.0     
🇺🇸  
BoBoiBoy: The Movie (2016)
    1.5     
🇲🇾  capeshit  
Bo Burnham: Inside (2021)
    3.5     
🇺🇸  
Bo Burnham: Make Happy (2016)
    2.5     
🇺🇸  
Bo Burnham: What (2013)
    3.0     
🇺🇸  
Das Boot (1981)
    4.5     
🇩🇪  endurance trial for my bladder  
Brave (2012)
    3.0     
🇺🇸  pixar  
Captain America: The Winter Soldier (2014)
    3.5     
🇺🇸  capeshit  
Capybara Walking (1887)
    3.5     
🇬🇧  short  
Cars (2006)
    3.5     
🇺🇸  pixar  
Cars 2 (2011)
    3.0     
🇺🇸  pixar  
Central Intelligence (2016)
    2.5     
🇺🇸  
Un chien andalou [An Andalusian Dog] (1929)
    3.0     
🇫🇷  short  thrift shop intro  
Cinderella (2015)
    2.5     
🇺🇸  
Cloudy With a Chance of Meatballs (2009)
    3.5     
🇺🇸  
Cloudy With a Chance of Meatballs 2 (2013)
    3.0     
🇺🇸  
Coco (2017)
    4.0     
🇺🇸  pixar  
Crazy For It (2012)
    3.5     
🇯🇵  short  
Cure (1997)
    5.0     
🇯🇵  
The Dark Knight (2008)
    4.0     
🇺🇸  capeshit  
Despicable Me (2010)
    2.5     
🇺🇸  
Despicable Me 2 (2013)
    3.0     
🇺🇸  
Django Unchained (2012)
    3.5     
🇺🇸  
Doctor Strange (2016)
    3.0     
🇺🇸  capeshit  
Drive (2011)
    3.5     
🇺🇸  
Eternal Sunshine of the Spotless Mind (2004)
    3.5     
🇺🇸  
Everything Everywhere All at Once (2022)
    4.5     
🇺🇸  🍿  
The Fall of Communism as Seen in Gay Pornography (1998)
    2.5     
🇺🇸  short  
Fanny och Alexander [Fanny and Alexander] (1982)
    4.0     
🇸🇪  endurance trial for my bladder  
Fargo (1996)
    4.0     
🇬🇧  🇺🇸  
The Fault in Our Stars (2014)
    2.0     
🇺🇸  
Festen [The Celebration] (1998)
    4.5     
🇩🇰  🇸🇪  
Finding Nemo (2003)
    4.0     
🇺🇸  pixar  
First Reformed (2017)
    4.0     
🇺🇸  
Five Feet Apart (2019)
    2.0     
🇺🇸  
Fresh Guacamole (2012)
    3.5     
🇺🇸  short  
Frozen (2013)
    2.5     
🇺🇸  
Gayniggers From Outer Space (1992)
    2.0     
🇩🇰  short  
Get Out (2017)
    4.0     
🇺🇸  
Hà Nội trong mắt ai [Hanoi Through Whose Eyes?] (1983)
    4.0     
🇻🇳  
High School Musical 3: Senior Year (2008)
    2.5     
🇺🇸  
The Holdovers (2023)
    4.0     
🇺🇸  
Hotel Transylvania (2012)
    3.0     
🇺🇸  
Hotel Transylvania 2 (2015)
    2.5     
🇺🇸  
Hot Fuzz (2007)
    4.5     
🇬🇧  
House (1977)
    3.5     
🇯🇵  thrift shop intro  
How to Train Your Dragon (2010)
    3.5     
🇺🇸  
The Incredibles (2004)
    3.5     
🇺🇸  capeshit  pixar  
Inglourious Basterds (2009)
    5.0     
🇩🇪  🇺🇸  
Inside Out (2015)
    4.0     
🇺🇸  pixar  
Inside Out 2 (2024)
    3.5     
🇺🇸  🍿  pixar  
It's Such a Beautiful Day (2012)
    4.5     
🇺🇸  
Jídlo [Food] (1992)
    3.0     
🇨🇿  short  thrift shop intro  
Johnny English Reborn (2011)
    2.5     
🇬🇧  
Jumanji (1995)
    3.5     
🇺🇸  
Juno (2007)
    3.0     
🇨🇦  🇺🇸  
The Karate Kid (2010)
    3.0     
🇨🇳  🇺🇸  
The King's Man (2021)
    2.0     
🇬🇧  🇺🇸  
The King's Speech (2010)
    3.0     
🇦🇺  🇬🇧  
Kingsman: The Golden Circle (2017)
    2.5     
🇬🇧  🇺🇸  
Kingsman: The Secret Service (2014)
    3.0     
🇬🇧  🇺🇸  
Kung Fu Panda (2008)
    3.5     
🇺🇸  
Kung Fu Panda 2 (2011)
    3.0     
🇺🇸  
La La Land (2016)
    4.0     
🇺🇸  
The Lion King (1994)
    4.0     
🇺🇸  
Madagascar (2005)
    3.0     
🇺🇸  
Madagascar 3: Europe's Most Wanted (2012)
    2.5     
🇺🇸  
Manchester by the Sea (2016)
    4.5     
🇺🇸  
Mắt biếc [Dreamy Eyes] (2019)
    2.0     
🇻🇳  🍿  
The Maze Runner (2014)
    2.5     
🇺🇸  
Meet the Robinsons (2007)
    3.5     
🇺🇸  
Megamind (2010)
    3.5     
🇺🇸  capeshit  
Memento (2000)
    4.5     
🇺🇸  
Microsoft Windows 95 Video Guide (1995)
    3.0     
🇺🇸  
Monster House (2006)
    3.0     
🇺🇸  
Monsters, Inc. (2001)
    4.0     
🇺🇸  pixar  
Monsters University (2013)
    3.5     
🇺🇸  pixar  
Moonlight (2016)
    4.0     
🇺🇸  
Mr. Bean's Holiday (2007)
    4.5     
🇩🇪  🇫🇷  🇬🇧  
Paddington (2014)
    3.5     
🇬🇧  
Past Lives (2023)
    3.5     
🇰🇷  🇺🇸  🍿  
Phantom Thread (2017)
    3.5     
🇺🇸  
Phineas and Ferb: Across the Second Dimension (2011)
    4.0     
🇺🇸  
The Princess and the Frog (2009)
    3.5     
🇺🇸  
The Pursuit of Happyness (2006)
    3.0     
🇺🇸  
Ratatouille (2007)
    4.5     
🇺🇸  pixar  
Rejected (2000)
    3.5     
🇺🇸  short  
RR (2007)
    3.5     
🇺🇸  
Shaun of the Dead (2004)
    4.0     
🇬🇧  
The Shawshank Redemption (1994)
    4.0     
🇺🇸  
A Short Vision (1956)
    3.5     
🇬🇧  short  
Sky High (2005)
    3.0     
🇺🇸  capeshit  
Snow White and the Seven Dwarfs (1937)
    3.5     
🇺🇸  
The Social Network (2010)
    3.5     
🇺🇸  this poster sucks  
Spider-Man (2002)
    3.5     
🇺🇸  capeshit  
Spider-Man 2 (2004)
    3.5     
🇺🇸  capeshit  
Spider-Man 3 (2007)
    2.5     
🇺🇸  capeshit  
Spider-Man: Into the Spider-Verse (2018)
    4.0     
🇺🇸  capeshit  
STAND BY ME ドラえもん [Stand by Me Doraemon] (2014)
    2.0     
🇯🇵  
Star Wars: The Force Awakens (2015)
    2.5     
🇺🇸  
The Strange Thing About the Johnsons (2011)
    0.5     
🇺🇸  short  
Surf's Up (2007)
    3.5     
🇺🇸  
Tangled (2010)
    3.0     
🇺🇸  
Taxi Driver (1976)
    5.0     
🇺🇸  
There Will Be Blood (2007)
    5.0     
🇺🇸  
Too Many Cooks (2014)
    3.5     
🇺🇸  short  thrift shop intro  
Toy Story (1995)
    4.0     
🇺🇸  pixar  
Toy Story 2 (1999)
    3.5     
🇺🇸  pixar  
Toy Story 3 (2010)
    4.0     
🇺🇸  pixar  
Tuổi dại [Green Age] (1974)
    3.0     
🇻🇳  
Uncut Gems (2019)
    4.0     
🇺🇸  
Up (2009)
    4.0     
🇺🇸  pixar  
WALL·E (2008)
    4.0     
🇺🇸  pixar  
Whiplash (2014)
    4.5     
🇺🇸  
Who Killed Captain Alex? (2010)
    4.5    
🇺🇬  
Wreck-It Ralph (2012)
    3.0     
🇺🇸  
Zootopia (2016)
    3.5     
🇺🇸  
Иди и смотри [Come and See] (1985)
    4.0     
🇧🇾  🇷🇺  
جدایی نادر از سیمین [A Separation] (2011)
    4.0     
🇮🇷  
অপুর সংসার [The World of Apu] (1959)
    4.0     
🇮🇳  
দুই [Two] (1964)
    3.5     
🇮🇳  short  
あさがおと加瀬さん [Kase-san and Morning Glories] (2018)
    3.5     
🇯🇵  
あずまんが大王（劇場短編） [Azumanga Daioh: The Very Short Movie] (2001)
    3.5     
🇯🇵  short  
オーディション [Audition] (1999)
    3.5     
🇯🇵  
映画 けいおん! [K-ON! Movie] (2011)
    4.0     
🇯🇵  
すばらしき世界 [Under the Open Sky] (2020)
    3.0     
🇯🇵  
ソナチネ [Sonatine] (1993)
    4.5     
🇯🇵  
タンポポ [Tampopo] (1985)
    3.5     
🇯🇵  
チャイム [Chime] (2024)
    3.5     
🇯🇵  
つみきのいえ [The House of Small Cubes] (2008)
    3.5     
🇯🇵  short  
トウキョウソナタ [Tokyo Sonata] (2008)
    4.5     
🇭🇰  🇯🇵  🇳🇱  
ドライブ・マイ・カー [Drive My Car] (2021)
    4.0     
🇯🇵  
パーフェクトブルー [Perfect Blue] (1997)
    5.0     
🇯🇵  
ハッピーアワー [Happy Hour] (2015)
    4.0     
🇯🇵  endurance trial for my bladder  
もののけ姫 [Princess Mononoke] (1997)
    4.5     
🇯🇵  
ユリイカ [Eureka] (2000)
    4.5     
🇯🇵  endurance trial for my bladder  
ラブライブ！The School Idol Movie [Love Live! The School Idol Movie] (2015)
    3.0     
🇯🇵  
リズと青い鳥 [Liz and the Blue Bird] (2018)
    4.0     
🇯🇵  
リリイ・シュシュのすべて [All About Lily Chou-Chou] (2001)
    3.5     
🇯🇵  
一一 [Yi Yi] (2000)
    5.0     
🇯🇵  🇹🇼  
万引き家族 [Shoplifters] (2018)
    4.0     
🇯🇵  
功夫 [Kung Fu Hustle] (2004)
    4.0     
🇨🇳  🇭🇰  
千と千尋の神隠し [Spirited Away] (2001)
    4.0     
🇯🇵  
千年女優 [Millennium Actress] (2001)
    4.0     
🇯🇵  
君の名は。 [Your Name.] (2016)
    4.0     
🇯🇵  
國產凌凌漆 [From Beijing With Love] (1994)
    2.5     
🇭🇰  
大紅燈籠高高掛 [Raise the Red Lantern] (1991)
    4.0     
🇨🇳  🇭🇰  
大象席地而坐 [An Elephant Sitting Still] (2018)
    4.5     
🇨🇳  endurance trial for my bladder  
幻の光 [Maborosi] (1995)
    4.0     
🇯🇵  
怪物 [Monster] (2023)
    4.5     
🇯🇵  🍿  
愛のむきだし [Love Exposure] (2008)
    3.5     
🇯🇵  endurance trial for my bladder  
新世紀エヴァンゲリオン劇場版 Air/まごころを、君に [The End of Evangelion] (1997)
    4.0     
🇯🇵  
歩いても 歩いても [Still Walking] (2008)
    4.0     
🇯🇵  
涼宮ハルヒの消失 [The Disappearance of Haruhi Suzumiya] (2010)
    4.0     
🇯🇵  
火垂るの墓 [Grave of the Fireflies] (1988)
    4.5     
🇯🇵  
牯嶺街少年殺人事件 [A Brighter Summer Day] (1991)
    4.5     
🇹🇼  endurance trial for my bladder  
聲の形 [A Silent Voice] (2016)
    3.5     
🇯🇵  
賭俠 [God of Gamblers II] (1991)
    3.0     
🇭🇰  
鉄男 [Tetsuo: The Iron Man] (1989)
    3.5     
🇯🇵  thrift shop intro  
青春ブタ野郎はゆめみる少女の夢を見ない [Rascal Does Not Dream of a Dreaming Girl] (2019)
    3.0     
🇯🇵  `

    // Split the input into lines
    lines := strings.Split(input, "\n")

    // Create a CSV writer
    csvFile, err := os.Create("movies.csv")
    if err != nil {
        fmt.Println("Error creating CSV file:", err)
        return
    }
    defer csvFile.Close()

    writer := csv.NewWriter(csvFile)
    defer writer.Flush()

    // Write header
    header := []string{"Title", "Year", "Rating", "Country", "Comments"}
    err = writer.Write(header)
    if err != nil {
        fmt.Println("Error writing header to CSV:", err)
        return
    }

    fmt.Println("Title,Year,Rating")
    // Process each movie block (3 lines per movie)
    for i := 0; i < len(lines); i += 3 {
        if i+2 >= len(lines) {
            continue // Skip incomplete or unexpected lines
        }

        // Parse title and year
        titleYear := strings.TrimSpace(lines[i])
        title := parseTitle(titleYear)
        year := parseYear(titleYear)

        // Parse rating
        rating := parseRating(lines[i+1])

        // Parse country and comments

        // Write to CSV
        _, err := fmt.Printf("\"%s\",%s,%s\n", title, year, rating)
        if err != nil {
            fmt.Println("Error writing record to CSV:", err)
            return
        }
    }

    //fmt.Println("CSV file successfully created: movies.csv")
}

func parseTitle(titleYear string) string {
    // Example titleYear: "大象席地而坐 [An Elephant Sitting Still] (2018)"
    start := strings.Index(titleYear, "[")
    end := strings.Index(titleYear, "]")
    if start != -1 && end != -1 && start < end {
        return strings.TrimSpace(titleYear[start+1 : end])
    }
    end = strings.Index(titleYear, "(") - 1
    return strings.TrimSpace(titleYear[:end])
}

func parseYear(titleYear string) string {
    // Example titleYear: "大象席地而坐 [An Elephant Sitting Still] (2018)"
    start := strings.Index(titleYear, "(")
    end := strings.Index(titleYear, ")")
    if start != -1 && end != -1 && start < end {
        return strings.TrimSpace(titleYear[start+1 : end])
    }
    return ""
}

func parseRating(ratingLine string) string {
    // Example ratingLine: "    4.5     "
    fields := strings.Fields(ratingLine)
    if len(fields) > 0 {
        return strings.TrimSpace(fields[0])
    }
    return ""
}

func parseCountryComments(countryCommentsLine string) CountryComments {
    // Example countryCommentsLine: "🇨🇳  endurance of my bladder"
    var countryComments CountryComments
    fields := strings.Fields(countryCommentsLine)
    if len(fields) > 0 {
        countryComments.Country = fields[0]
    }
    if len(fields) > 1 {
        countryComments.Comments = strings.Join(fields[1:], " ")
    }
    return countryComments
}

type CountryComments struct {
    Country  string
    Comments string
}
