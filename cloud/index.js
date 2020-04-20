var restify = require('restify');

var errs = require('restify-errors');
var server = restify.createServer();

server.use(restify.plugins.bodyParser({mapParams: true}));

var powerstate = true;
var effect = "sparkle";
var colors = [0, 0, 0, 0, 0, 0];
var num_colors = 1;
var index = 1;
var brightness = 100;
var speed = 128;

// 0 or 1 to multiply and then a number to add so a single table

const speed_action_values = {
    "normal speed": { mul: 0, add: 128 },
    "fastest": { mul: 0, add: 255 },
    "slowest": { mul: 0, add: 32 },
    "fast": { mul: 0, add: 200 },
    "slow": { mul: 0, add: 100 },
    "faster": { mul: 1, add: 48 },
    "slower": { mul: 1, add: -48 },
    "slow down": { mul: 1, add: -48 },
    "speed up": { mul: 1, add: 48 }
};

server.post('/set', function state(req, res, next) {
    console.log("POST:" + JSON.stringify(req.params));
    res.send({
        'status': 'ok' 
    });
    var p = req.params;
    let index_increment = 0;

    if(p.speed_action) {
        index_increment = 1;
        if(speed_action_values.hasOwnProperty(p.speed_action)) {
            let action = speed_action_values[p.speed_action];   // TODO (chs): untested...
            speed = speed * action.mul + action.add;
        }
        if(speed < 32) {
            speed = 32;
        }
        else if(speed > 255) {
            speed = 255;
        }
    }

    if(p.hasOwnProperty("brightness")) {
        index_increment = 1;
        brightness = p.brightness;
    }

    if(p.hasOwnProperty("brightness_delta")) {
        index_increment = 1;
        brightness += p.brightness_delta;
        index_increment = 1;
    }

    if(p.hasOwnProperty("powerstate")) {
        index_increment = 1;
        powerstate = p.powerstate;
        console.log("powerstate " + powerstate);
    }

    if(p.action) {
        if(p.action !== 'go' || p.color === undefined) {
            effect = p.action;
            powerstate = true;
            index_increment = 1;
        }
    }

    if(p.colors) {
        index_increment = 1;
        colors = p.colors;
        num_colors = p.colors.length;
        powerstate = true;
    }

    if(brightness < 0) {
        brightness = 0;
    }
    else if(brightness > 100) {
        brightness = 100;
    }

    index += index_increment;
    return next();
});

var gets = 0;

server.get('/state', function get_state(req, res, next) {
    var body = {
        'status': 'ok',
        'index': index,
        'powerstate': powerstate,
        'effect': effect,
        'colors': colors,
        'num_colors': num_colors,
        'brightness': brightness,
        'speed': speed
    };
    console.log(new Date() + " GET: " + JSON.stringify(body) );
    gets += 1;
    if((gets % 10) == 0) {
        console.log("Keeping an eye on it...");
    }
    res.send(body);
    return next();
});

process.on('SIGINT', function() {
    process.exit();
});

server.get('/', function root(req, res, next) {
    res.send(404, new errs.NotFoundError("Huh?"));
    return next();
});

server.listen(3000, function() {
  console.log('%s listening at %s', server.name, server.url);
});
