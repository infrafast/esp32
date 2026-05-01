#!/usr/bin/env node
const dgram = require('node:dgram');

function parseArgs(argv) {
  const opts = { timeoutMs: 2000 };

  for (let i = 0; i < argv.length; i += 1) {
    const a = argv[i];
    const next = argv[i + 1];
    switch (a) {
      case '--host': opts.host = next; i += 1; break;
      case '--port': opts.port = Number(next); i += 1; break;
      case '--protocol':
      case '--console': opts.protocol = (next || '').toLowerCase(); i += 1; break;
      case '--command': opts.command = next; i += 1; break;
      case '--int': opts.intValue = Number(next); i += 1; break;
      case '--float': opts.floatValue = Number(next); i += 1; break;
      case '--string': opts.stringValue = next; i += 1; break;
      case '--timeout': opts.timeoutMs = Number(next); i += 1; break;
      default:
        if (a.startsWith('-')) throw new Error(`Unknown argument: ${a}`);
    }
  }

  if (!opts.host || !opts.port || !opts.command) {
    throw new Error('Missing required args: --host --port --command');
  }
  if (opts.protocol !== 'x32' && opts.protocol !== 'xairosc') {
    throw new Error('Protocol must be --protocol x32 or --protocol xairosc');
  }
  return opts;
}

function padOscString(input) {
  const base = Buffer.from(`${input}\0`, 'utf8');
  const pad = (4 - (base.length % 4)) % 4;
  return Buffer.concat([base, Buffer.alloc(pad)]);
}

function buildOscPacket(opts) {
  const address = opts.command.startsWith('/') ? opts.command : `/${opts.command}`;
  const values = [];
  if (opts.intValue !== undefined && !Number.isNaN(opts.intValue)) {
    const b = Buffer.alloc(4); b.writeInt32BE(opts.intValue); values.push({ tag: 'i', data: b });
  }
  if (opts.floatValue !== undefined && !Number.isNaN(opts.floatValue)) {
    const b = Buffer.alloc(4); b.writeFloatBE(opts.floatValue); values.push({ tag: 'f', data: b });
  }
  if (opts.stringValue !== undefined) values.push({ tag: 's', data: padOscString(opts.stringValue) });

  const typeTags = `,${values.map((v) => v.tag).join('')}`;
  return Buffer.concat([padOscString(address), padOscString(typeTags), ...values.map((v) => v.data)]);
}

function normalizeCommand(protocol, command) {
  const base = command.startsWith('/') ? command : `/${command}`;
  if (protocol === 'x32' || protocol === 'xairosc') return base;
  return base;
}

async function main() {
  const opts = parseArgs(process.argv.slice(2));
  const command = normalizeCommand(opts.protocol, opts.command);
  const packet = buildOscPacket({ ...opts, command });
  const socket = dgram.createSocket('udp4');

  socket.on('message', (msg, rinfo) => {
    console.log(`Reply from ${rinfo.address}:${rinfo.port} -> ${msg.toString('hex')}`);
  });

  await new Promise((resolve, reject) => {
    socket.send(packet, opts.port, opts.host, (err) => {
      if (err) reject(err);
      else { console.log(`Sent ${packet.length} bytes to ${opts.host}:${opts.port} (${opts.protocol}) : ${command}`); resolve(); }
    });
  });

  setTimeout(() => socket.close(), opts.timeoutMs);
}

main().catch((e) => { console.error('[openai-remote] Error:', e.message); process.exit(1); });
