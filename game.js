const canvas = document.getElementById('gameCanvas');
const ctx = canvas.getContext('2d');
canvas.width = window.innerWidth;
canvas.height = window.innerHeight;

// WebSocket connection
const ws = new WebSocket('ws://localhost:3000');

let players = [];
let grid = [];
let myId = '';
let myPlayer = null;

// WebSocket event handlers
ws.onopen = () => {
    console.log('Connected to game server');
    
};

ws.onclose = () => {
    console.log('Disconnected from game server');
};

ws.onerror = (error) => {
    console.error('WebSocket error:', error);
};

ws.onmessage = (event) => {
    const data = JSON.parse(event.data);
    console.log('Received:', data);
    
    if (data.type === 'init') {
        handleInit(data);
    } else if (data.type === 'newPlayer') {
        handleNewPlayer(data);
    } else if (data.type === 'playerDisconnected') {
        handlePlayerDisconnected(data.id);
    } else if (data.type === 'playerDead') {
        handlePlayerDead(data.id);
    } else if (data.type === 'update') {
        handleUpdate(data);
    }
};

// Game state handlers
function handleInit(data) {
    players = data.players;
    grid = data.grid;
    myId = data.yourId;
    myPlayer = players.find(x => x.id === myId);
    console.log("Game initialized", data);
}

function handleNewPlayer(data) {
    players.push(data.player);
    console.log("New player joined:", data.player);
}

function handlePlayerDisconnected(id) {
    players = players.filter(x => x.id !== id);
    console.log("Player disconnected:", id);
}

function handlePlayerDead(id) {
    const player = players.find(x => x.id === id);
    if (player) {
        player.isAlive = false;
    }
    console.log("Player died:", id);
}

function handleUpdate(data) {
    players = data.players;
    grid = data.grid;
    let direction = myPlayer.direction
    myPlayer = players.find(x => x.id === myId);
    myPlayer.direction = direction
    document.getElementById("playerCount").innerHTML = players.length;
    document.getElementById("playerScore").innerHTML = myPlayer.score;
    console.log("Game updated");
}

// Управление
window.addEventListener('keydown', (e) => {
    if (!myPlayer || !myPlayer.isAlive) return;

    if (e.key === 'ArrowUp') myPlayer.direction = 0; // myPlayer.y -= myPlayer.speed;
    if (e.key === 'ArrowDown') myPlayer.direction = 1;//myPlayer.y += myPlayer.speed;
    if (e.key === 'ArrowLeft') myPlayer.direction = 2;//myPlayer.x -= myPlayer.speed;
    if (e.key === 'ArrowRight') myPlayer.direction = 3;//myPlayer.x += myPlayer.speed;
    sendMove()
    //socket.emit('move', { x: myPlayer.x, y: myPlayer.y, direction: myPlayer.direction });
});

function sendMove() {
    if (ws.readyState === WebSocket.OPEN && myPlayer) {
        ws.send(JSON.stringify({
            type: 'move',
            x: myPlayer.x,
            y: myPlayer.y,
            direction: myPlayer.direction
        }));
    }
}
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
        sendMove();
       // socket.emit('move', { x: myPlayer.x, y: myPlayer.y, direction: myPlayer.direction });
    }

    requestAnimationFrame(draw);
}

draw();