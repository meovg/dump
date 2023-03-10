async function wait2seconds() {
	return new Promise(resolve => {
		setTimeout(() => {
			// console.log("2 sec");
			resolve([2, 4, 6]);
		}, 2000);
	});
}

async function wait1seconds() {
	return new Promise(resolve => {
		setTimeout(() => {
			// console.log("1 sec");
			resolve([1, 2, 3]);
		}, 1000);
	});
}

async function doshit() {
	let a = wait2seconds();
	let b = wait1seconds();
	console.log(await a);
	console.log(await b);
}

// wait2seconds();
console.log("am i faster?");
doshit();