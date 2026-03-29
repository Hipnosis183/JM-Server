import { GameEntry, GetMessage, GetName, GetRanking, GetReplay, ScoreEntry } from './services/service.js';
import { outputRequest } from './output.js';

// Manage server connection.
export const serverConnection = async (request, response) => {
  // Output packet data.
  outputRequest(request);
  // Endpoint parse.
  switch (request.url.split('?')[0]) {
    case '/JM_test/service/GameEntry': {
      GameEntry(request, response); break;
    }
    case '/JM_test/service/GetMessage': {
      GetMessage(request, response); break;
    }
    case '/JM_test/service/GetName': {
      GetName(request, response); break;
    }
    case '/JM_test/service/GetRanking': {
      GetRanking(request, response); break;
    }
    case '/JM_test/service/GetReplay': {
      GetReplay(request, response); break;
    }
    case '/JM_test/service/ScoreEntry': {
      await parseMultipart(request);
      ScoreEntry(request, response); break;
    }
    default: {
      // Empty response on unidentified endpoint.
      response.end(); break;
    }
  }
}

// Parse multipart chunks.
export const parseMultipart = (request) => {
  return new Promise((resolve, reject) => {
    const chunks = [];
    request.on('data', (chunk) => chunks.push(chunk));
    request.on('end', () => {
      const buffer = Buffer.concat(chunks);
      // Parse headers.
      const boundary = '--' + request.headers['content-type'].split('boundary=')[1];
      const hStart = buffer.indexOf(boundary) + boundary.length;
      const hEnd = buffer.indexOf('\r\n\r\n', hStart);
      const headers = buffer.subarray(hStart, hEnd).toString();
      const name = headers.match(/name="([^"]+)"/)[1];
      const filename = headers.match(/filename="([^"]+)"/)[1];
      // Parse body.
      const bStart = hEnd + 4;
      const bEnd = buffer.indexOf(boundary, bStart) - 2;
      const body = buffer.subarray(bStart, bEnd);
      const size = body.length;
      request.file = { name, filename, body, size };
      resolve();
    });
  });
}