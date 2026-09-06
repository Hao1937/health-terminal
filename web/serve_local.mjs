import fs from "node:fs";
import http from "node:http";
import https from "node:https";
import path from "node:path";
import { fileURLToPath } from "node:url";

const root = path.dirname(fileURLToPath(import.meta.url));
const options = { host: "127.0.0.1", port: 8000, cert: "", key: "" };
for (let i = 2; i < process.argv.length; i += 1) {
  const name = process.argv[i];
  if (name === "--host") options.host = process.argv[++i];
  else if (name === "--port") options.port = Number(process.argv[++i]);
  else if (name === "--cert") options.cert = process.argv[++i];
  else if (name === "--key") options.key = process.argv[++i];
  else throw new Error(`未知参数: ${name}`);
}
if (!Number.isInteger(options.port) || options.port < 1 || options.port > 65535) {
  throw new Error("--port 必须为 1-65535 的整数");
}
if (Boolean(options.cert) !== Boolean(options.key)) {
  throw new Error("--cert 与 --key 必须同时提供");
}

const mime = { ".html": "text/html; charset=utf-8", ".js": "text/javascript; charset=utf-8",
  ".css": "text/css; charset=utf-8", ".svg": "image/svg+xml", ".json": "application/json" };
function handler(request, response) {
  const requestPath = decodeURIComponent(new URL(request.url, "http://localhost").pathname);
  const relative = requestPath === "/" ? "dashboard.html" : requestPath.replace(/^\/+/, "");
  const filename = path.resolve(root, relative);
  const blocked = new Set([".pem", ".key", ".pfx", ".p12", ".crt", ".cer"]);
  if ((filename !== root && !filename.startsWith(root + path.sep)) ||
      blocked.has(path.extname(filename).toLowerCase())) {
    response.writeHead(403).end("Forbidden"); return;
  }
  fs.readFile(filename, (error, data) => {
    if (error) { response.writeHead(error.code === "ENOENT" ? 404 : 500).end("Not found"); return; }
    response.writeHead(200, { "Content-Type": mime[path.extname(filename)] || "application/octet-stream",
      "Cache-Control": "no-store" });
    response.end(data);
  });
}

const tls = options.cert ? { cert: fs.readFileSync(options.cert), key: fs.readFileSync(options.key) } : null;
const server = tls ? https.createServer(tls, handler) : http.createServer(handler);
server.listen(options.port, options.host, () => {
  const scheme = tls ? "https" : "http";
  console.log(`Dashboard: ${scheme}://${options.host}:${options.port}/dashboard.html`);
});
