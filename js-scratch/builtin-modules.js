const os = require('os')

// info about current user
const user = os.userI nfo()
console.log(user)

// uptime
console.log(`system uptime is ${os.uptime()} seconds`)

const currentos = {
    name: os.type(),
    release: os.release(),
    totalmem: os.totalmem(),
    freemem: os.freemem()
}
console.log(currentos)

const path = require('path')

console.log(path.sep) // path separator

console.log(__dirname)
const h = path.resolve(__dirname, 'deez', 'nahts', 'balls.cc')
console.log(h)