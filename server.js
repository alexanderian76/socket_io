const express = require('express');
const http = require('http');
const socketIO = require('socket.io');

const app = express();
const server = http.createServer(app);
const io = socketIO(server, {
  cors: {
    origin: "*", // Разрешаем все домены (для разработки)
    methods: ["GET", "POST"]
  }
});

app.use((req, res, next) => {
  res.header("Access-Control-Allow-Origin", "*");
  res.header("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
  next();
});

const PORT = 3000;

// Храним всех игроков и их данные
let players = [];
let grid = []; // Общая карта

// При подключении нового игрока
io.on('connection', (socket) => {
    console.log(`New player connected: ${socket.id}`);
    // Создаём нового игрока
    players.push({
        id: socket.id,
        x: Math.random() * 800,
        y: Math.random() * 600,
        color: `hsl(${Math.random() * 360}, 100%, 50%)`,
        radius: 5,
        speed: 0.5,
        trail: [],
        isAlive: true,
        direction: Math.floor(Math.random(4)),
        score: 0
    });

    // Отправляем текущее состояние игры новому игроку
    socket.emit('init', { players, grid, yourId: socket.id });

    // Оповещаем остальных о новом игроке
    socket.broadcast.emit('newPlayer', { id: socket.id, player: players.find(x => x.id === socket.id) });

    // Обработка движения игрока
    socket.on('move', (data) => {
        if (!players.some(x => x.id === socket.id) || !players.find(x => x.id === socket.id).isAlive) return;

        const index = players.findIndex(x => x.id === socket.id)
        players[index].x = data.x;
        players[index].y = data.y;
        players[index].direction = data.direction;

        // Обновляем след на сервере
        const gridX = Math.floor(data.x / 10) * 10;
        const gridY = Math.floor(data.y / 10) * 10;
        

        // Проверка столкновений
       // if (grid[`${gridX},${gridY}`] && grid[`${gridX},${gridY}`] !== players[socket.id].color) {
       if(grid.some(x => x.gridX == gridX && x.gridY == gridY && x.color !== players.find(x => x.id === socket.id).color)) {
            console.log("ASDASDDA")
            players[index].isAlive = false;
            socket.emit('dead');
            socket.broadcast.emit('playerDead', socket.id);
            grid = grid.filter(x => x.color !== players.find(x => x.id === socket.id).color)
            const i = players.findIndex(x => x.color === grid.find(x => x.gridX == gridX && x.gridY == gridY).color)
            players[i].score += players.find(x => x.id === socket.id).score
        }
        else
            grid.push({
                gridX: gridX,
                gridY: gridY,
                color: players.find(x => x.id === socket.id).color
            })
            players[index].score++;
        
        
        // Отправляем обновление всем игрокам
        io.emit('update', { players, grid });
    });

    // При отключении игрока
    socket.on('disconnect', () => {
        console.log(`Player disconnected: ${socket.id}`);
        players = players.filter(x => x.id !== socket.id);
        io.emit('playerDisconnected', socket.id);
    });
});

server.listen(PORT, () => {
    console.log(`Server running on http://localhost:${PORT}`);
});