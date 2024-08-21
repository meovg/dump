package main

import (
	"image"
	"image/color"
	"image/gif"
	"image/png"
	"io"
	"log"
	"math"
	"math/cmplx"
	"math/rand"
	"net/http"
	"strconv"
	"strings"
)

func parseMandelbrotRequest(r *http.Request) (int, uint8, uint8) {
	var (
		dim            int   = 1024
		contrast, iter uint8 = 15, 200
		tmp            int64
		err            error
	)

	query := r.URL.RawQuery
	queryArr := strings.Split(query, "&")

	for _, q := range queryArr {
		p := strings.Split(q, "=")
		if len(p) < 2 {
			continue
		}

		if tmp, err = strconv.ParseInt(p[1], 10, 64); err != nil {
			continue
		}

		switch p[0] {
		case "dim":
			if tmp <= 4000 {
				dim = int(tmp)
			}
		case "contrast":
			if tmp < 255 {
				contrast = uint8(tmp)
			}
		case "iter":
			if tmp < 500 {
				iter = uint8(tmp)
			}
		}
	}

	return dim, contrast, iter
}

func mandelbrotCalc(c complex128, contrast uint8, iter uint8) color.Color {
	var z complex128
	for n := uint8(0); n < iter; n++ {
		z = z*z + c
		if cmplx.Abs(z) > 2 {
			return color.Gray{255 - contrast*n}
		}
	}

	return color.Black
}

func mandelbrot(out io.Writer, dim int, contrast uint8, iter uint8) {
	const xmin, ymin, xmax, ymax = -2, -2, 2, 2

	img := image.NewRGBA(image.Rect(0, 0, dim, dim))

	for py := 0; py < dim; py++ {
		y := float64(py)/float64(dim)*(ymax-ymin) + ymin
		for px := 0; px < dim; px++ {
			x := float64(px)/float64(dim)*(xmax-xmin) + xmin
			c := complex(x, y)
			img.Set(px, py, mandelbrotCalc(c, contrast, iter))
		}
	}

	png.Encode(out, img)
}

func lissajous(out io.Writer) {
	const (
		cycles     = 5     // number of complete x oscillator revolutions
		res        = 0.001 // angular resolution
		size       = 100   // image canvas covers [-size..+size]
		nframes    = 64    // number of animation frames
		delay      = 8     // delay between frames in 10ms units
		whiteIndex = 0
		blackIndex = 1
	)

	var palette = []color.Color{color.White, color.Black}

	freq := rand.Float64() * 3.0 // relative frequency of y oscillator
	anim := gif.GIF{LoopCount: nframes}
	phase := 0.0 // phase difference

	for i := 0; i < nframes; i++ {
		rect := image.Rect(0, 0, 2*size+1, 2*size+1)
		img := image.NewPaletted(rect, palette)

		for t := 0.0; t < cycles*2*math.Pi; t += res {
			x := math.Sin(t)
			y := math.Sin(t*freq + phase)
			img.SetColorIndex(size+int(x*size+0.5), size+int(y*size+0.5), blackIndex)
		}
		phase += 0.1
		anim.Delay = append(anim.Delay, delay)
		anim.Image = append(anim.Image, img)
	}

	gif.EncodeAll(out, &anim)
}

func main() {
	http.HandleFunc("/", func(w http.ResponseWriter, r *http.Request) {
		log.Printf("je baited xD")
		io.WriteString(w, "the game")
	})

	http.HandleFunc("/lissajous", func(w http.ResponseWriter, r *http.Request) {
		log.Printf("lissajous")
		lissajous(w)
	})

	http.HandleFunc("/mandelbrot", func(w http.ResponseWriter, r *http.Request) {
		dim, contrast, iter := parseMandelbrotRequest(r)
		log.Printf("mandelbrot dim:%d contrast:%d iter:%d\n", dim, contrast, iter)
		mandelbrot(w, dim, contrast, iter)
	})

	log.Fatal(http.ListenAndServe("localhost:8000", nil))
}
