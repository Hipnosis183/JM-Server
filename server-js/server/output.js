// Print common output data.
const outputHeader = (address) => {
  const datetime = new Date().toISOString().replace('T', ' ').substring(0, 19).replace(/-/g, '/');
  process.stdout.write(`\n${datetime.padEnd(20, ' ')} ${address.padEnd(15, ' ')} > `);
}

// Print server initialization data.
export const outputInit = () => {
  outputHeader('0.0.0.0');
  process.stdout.write('INIT\n');
}

// Print received request data.
export const outputRequest = (request) => {
  const url = request.url.split('?');
  outputHeader(request.socket.remoteAddress.slice(7));
  process.stdout.write(`${request.method.padEnd(6, ' ')} ${url[0]}\n`);
  if (url[1]) {
    process.stdout.write(`${'>'.padStart(38, ' ')} ${'QUERY'.padEnd(6, ' ')} ${url[1]}\n`);
  }
}