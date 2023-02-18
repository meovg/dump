const { readFile, writeFile } = require('fs')

readFile('./text/content1.txt', (err, result) => {
    if (err) {
        console.log(err)
        return
    }
    ichi = result
    readFile('./text/content2.txt', (err, result) => {
        if (err) {
            console.log(err)
            return
        }
        ni = result
        writeFile(
            './text/fatwa1.txt',
            `Sunny Blue by fishmans\n${ichi}: ${ni}`,
            { flag : 'a' },
            (err, result) => {
                if (err) {
                    console.log('dumbass writing')
                    return
                }
                console.log(`nvm you're done`)
            }
        )
    })
})