const axios = require('axios')

exports.handler = async function(event) {
    const promise = new Promise(function(resolve, reject) {
        axios.get('https://gridwatch.templar.co.uk/').then(response => {
            let responseObject = {
                demand: 0,
                frequency: 0,
                types: [],
            };
            const demandr = new RegExp(/<b>Demand (\d+\.\d+)GW<\/b>/img);
            let demandmatches = demandr.exec(response.data);
            if (demandmatches) {
                responseObject.demand = demandmatches[1];
            }
            const freqr = new RegExp(/<b>Frequency (\d+\.\d+)Hz<\/b>/img);
            let freqMatches = freqr.exec(response.data);
            if (freqMatches) {
                responseObject.frequency = freqMatches[1];
            }
            const typesr = new RegExp(/<center><b>(.*) (-?\d+\.\d+)GW<br>\((-?\d+\.\d+)%\)<\/b><\/center>/img);

            let typeMatches;

            while((typeMatches = typesr.exec(response.data)) !== null) {
                responseObject.types.push({
                    type: typeMatches[1],
                    output: typeMatches[2],
                    percent: typeMatches[3],
                });
            }
            resolve(responseObject);
        }).catch(err => {
            reject(err);
        });
    });
    return promise;
}
