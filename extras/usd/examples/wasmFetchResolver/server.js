// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

// This is a simple zero-config webserver used to power the Fetch API example.
const express = require('express');
const cors = require('cors');
const path = require('path');
const fs = require('fs');
const { program } = require('commander');


program
    .option('-d, --directory <path>', 'directory to serve files from', 'public')
    .option('-p, --port <number>', 'port number to start the server on', '8080')
    .parse(process.argv);

const options = program.opts();
const port = parseInt(options.port);
const directory = options.directory;
const directoryPath = path.join(__dirname, directory);


if (!fs.existsSync(directoryPath)) {
    throw new Error(`Directory does not exist: ${directoryPath}`);
}

const app = express();

app.use(cors());

// These custom headers are required to be present due to the use of -pthread
// Refer to: https://emscripten.org/docs/porting/pthreads.html
app.use((req, res, next) => {
    res.set('Cross-Origin-Embedder-Policy', 'require-corp');
    res.set('Cross-Origin-Opener-Policy', 'same-origin');
    next();
});

app.use((req, res, next) => {
    const timestamp = new Date().toISOString();
    console.log(`[${timestamp}] ${req.method} ${req.url}`);
    next();
});

app.get('/stages', (req, res) => {
    const stagesPath = path.join(__dirname, 'stages.json');
    if (fs.existsSync(stagesPath)) {
        res.sendFile(stagesPath);
    } else {
        res.sendStatus(404);
    }
});

app.use(express.static(directoryPath));

app.listen(port, () => {
    console.log(`Server is running on http://localhost:${port}`);
    console.log(`Serving files from: ${directoryPath}`);
});
