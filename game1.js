const canvas = document.getElementById('gameCanvas');
const ctx = canvas.getContext('2d');
canvas.width = window.innerWidth;
canvas.height = window.innerHeight;

// Подключение к серверу через Socket.io
const socket = io('http://localhost:3000', {
    path: "/",
    transports: ['websocket'], // Используем только WebSocket
    withCredentials: false, // Отключаем куки, если не нужны
    //closed: false
});

let players = [];
let grid = [];
let myId = '';
let myPlayer = null;
socket.on('connect', () => {
  console.log('Connected to server with ID:', socket.id);
});
// Получение начальных данных от сервера
socket.on('init', (data) => {
    console.log("INIT", data)
    players = data.players;
    grid = data.grid;
    myId = data.yourId;
    myPlayer = players.find(x => x.id === myId);
});
socket.on('*', (data) => {
    console.log("ANY DATA", data)
})
// Новый игрок подключился
socket.on('newPlayer', (data) => {
    console.log("newPlayer", data)
    players.push(data.player);
});

// Игрок отключился
socket.on('playerDisconnected', (id) => {
     console.log("playerDisconnected", id)
    players = players.filter(x => x.id !== id);
});

// Игрок умер
socket.on('playerDead', (id) => {
    if (players.some(x => x.id === id)) {
        players = players.map(x => {
            if (x.id === id)
                x.isAlive = false
            return x
        })
    };
});

// Обновление игры
socket.on('update', (data) => {
    players = data.players;
    grid = data.grid;
    myPlayer = players.find(x => x.id === myId)
    document.getElementById("playerCount").innerHTML = players.length
    document.getElementById("playerScore").innerHTML = myPlayer.score
});

// Управление
window.addEventListener('keydown', (e) => {
    if (!myPlayer || !myPlayer.isAlive) return;

    if (e.key === 'ArrowUp') myPlayer.direction = 0; // myPlayer.y -= myPlayer.speed;
    if (e.key === 'ArrowDown') myPlayer.direction = 1;//myPlayer.y += myPlayer.speed;
    if (e.key === 'ArrowLeft') myPlayer.direction = 2;//myPlayer.x -= myPlayer.speed;
    if (e.key === 'ArrowRight') myPlayer.direction = 3;//myPlayer.x += myPlayer.speed;
    socket.emit('move', { x: myPlayer.x, y: myPlayer.y, direction: myPlayer.direction });
});

// Отрисовка
function draw() {

    // ctx.beginPath()
    console.log(canvas.width, canvas.height)
    ctx.clearRect(0, 0, canvas.width, canvas.height);
    ctx.beginPath();
    //ctx.fillRect(canvas.width, canvas.height, 0, 0);
    // Рисуем сетку
    grid.forEach(key => {
        // const [x, y] = key.split(',').map(Number);
        ctx.fillStyle = key.color;
        ctx.fillRect(key.gridX, key.gridY, 10, 10);
    })

    // Рисуем всех игроков
    players.forEach(id => {
        const player = players.find(x => x.id === id.id);
        console.log(id)
        if (player && player.isAlive) {
            ctx.fillStyle = player.color;
            ctx.beginPath();
            ctx.arc(player.x, player.y, player.radius, 0, Math.PI * 2);
            ctx.fill();

            // Подпись над игроком
            ctx.fillStyle = 'black';
            ctx.font = '12px Arial';
            ctx.fillText(id.id.slice(0, 5), player.x - 15, player.y - 10);
        }
    })

    // Если игрок мёртв
    if (myPlayer && !myPlayer.isAlive) {
        ctx.fillStyle = 'red';
        ctx.font = '30px Arial';
        ctx.fillText('You Died!', canvas.width / 2 - 80, canvas.height / 2);
    }
    if (myPlayer) {
        console.log(myPlayer)
        switch (myPlayer.direction) {
            case 0:
                myPlayer.y -= myPlayer.speed;
                break;
            case 1:
                myPlayer.y += myPlayer.speed;
                break;
            case 2:
                myPlayer.x -= myPlayer.speed;
                break;
            case 3:
                myPlayer.x += myPlayer.speed;
                break;
        }
        // Отправляем движение на сервер
        socket.emit('move', { x: myPlayer.x, y: myPlayer.y, direction: myPlayer.direction });
    }

    requestAnimationFrame(draw);
}

draw();