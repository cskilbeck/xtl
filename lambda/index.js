/* eslint-disable func-names */
/* eslint-disable no-restricted-syntax */
/* eslint-disable no-loop-func */
/* eslint-disable consistent-return */
/* eslint-disable no-console */
/* eslint-disable max-len */
/* eslint-disable prefer-destructuring */

const Alexa = require('ask-sdk-core');
const req = require('request-promise');
const colors = require('./colors');

// factory reset
// proper wifi setup process on the ESP12 (WifiManager with captive portal)
// ZeroConf support in the Android app / autodiscover controllers
// save off state so it does the same when internet is down
// HSB model right down to the last stage in the STM103?

//////////////////////////////////////////////////////////////////////

function pad(s, size) {
    while (s.length < (size || 2)) {
        s = "0" + s;
    }
    return s;
}

//////////////////////////////////////////////////////////////////////

const rgb_from_color_name =
    function(color_name) {
    let r = colors.rgb_from_color_name(color_name);
    console.log(`Color '${color_name}' = ${pad(r.toString(16), 8)}`);
    return r;
}

//////////////////////////////////////////////////////////////////////

const rgb_from_hsv =
    function(hsb) {
    let rgb = colors.rgb_from_hsv(hsb);
    console.log(`hsb:${JSON.stringify(hsb)} => ${JSON.stringify(rgb)}`);
    return (rgb.r << 16) | (rgb.g << 8) | rgb.b;
}

//////////////////////////////////////////////////////////////////////
// INTENT HANDLERS

//////////////////////////////////////////////////////////////////////
// LaunchRequest

const LaunchRequestHandler = {
    canHandle(handlerInput) {
        return handlerInput.requestEnvelope.request.type === 'LaunchRequest';
    },
    handle(handlerInput) { return handlerInput.responseBuilder.getResponse(); },
};

//////////////////////////////////////////////////////////////////////
// solid_intent

const SolidHandler = {
    canHandle(handlerInput) {
        const request = handlerInput.requestEnvelope.request;
        return request.type === 'IntentRequest' &&
               request.intent.name === 'solid_intent';
    },
    async handle(handlerInput) {
        const filledSlots = handlerInput.requestEnvelope.request.intent.slots;
        const slotValues = getSlotValues(filledSlots);
        let text = "They don't know how to do that"
        if (slotValues.action && slotValues.action.isValidated) {
            let params = {"colors" : []};
            if (slotValues.action.resolved !== 'go') {
                params.action = slotValues.action.resolved;
            }
            if (slotValues.color_one && slotValues.color_one.isValidated) {

                params.colors.push(
                    rgb_from_color_name(slotValues.color_one.resolved));

                if (slotValues.color_two && slotValues.color_two.isValidated) {
                    params.colors.push(
                        rgb_from_color_name(slotValues.color_two.resolved));
                }
                if (slotValues.color_three &&
                    slotValues.color_three.isValidated) {
                    params.colors.push(
                        rgb_from_color_name(slotValues.color_three.resolved));
                }
                await turn(params);
                text = "ok"
            } else {
                text = "They don't know about the color " +
                       filledSlots.color_one.value + ", sorry";
            }
        }
        else {
            text =
                "I'm sorry, The Christmas Tree Lights are confused. And not just a bit. I mean, like, they basically have no idea what's going on. Perhaps you should say something simpler.";
        }
        return handlerInput.responseBuilder.speak(text).getResponse();
    }
};

//////////////////////////////////////////////////////////////////////

const SpeedHandler = {
    canHandle(handlerInput) {
        const request = handlerInput.requestEnvelope.request;
        return request.type === 'IntentRequest' &&
               request.intent.name === 'speed_intent';
    },
    async handle(handlerInput) {
        const filledSlots = handlerInput.requestEnvelope.request.intent.slots;
        const slotValues = getSlotValues(filledSlots);
        let action = null;
        if (slotValues.speed_percent && slotValues.speed_percent.isValidated) {
            await turn({"speed_set" : slotValues.speed_action.resolved});
        } else if (slotValues.speed_action &&
                   slotValues.speed_action.isValidated) {
            action = slotValues.speed_action.resolved;
        } else if (slotValues.speed_change &&
                   slotValues.speed_change.isValidated) {
            action = slotValues.speed_change.resolved;
        }
        if (action) {
            await turn({"speed_action" : action});
        }
        return handlerInput.responseBuilder.speak('ok').getResponse();
    }
};

//////////////////////////////////////////////////////////////////////

const ActionHandler = {
    canHandle(handlerInput) {
        const request = handlerInput.requestEnvelope.request;
        return request.type === 'IntentRequest' &&
               request.intent.name === 'action_intent';
    },
    async handle(handlerInput) {
        const filledSlots = handlerInput.requestEnvelope.request.intent.slots;
        const slotValues = getSlotValues(filledSlots);
        if (slotValues.action && slotValues.action.isValidated) {
            await turn({"action" : slotValues.action.resolved});
        }
        return handlerInput.responseBuilder.speak('ok').getResponse();
    },
};

//////////////////////////////////////////////////////////////////////
// Fallback

const FallbackHandler = {
    canHandle(handlerInput) {
        const request = handlerInput.requestEnvelope.request;
        return request.type === 'IntentRequest' &&
               request.intent.name === 'AMAZON.FallbackIntent';
    },
    handle(handlerInput) {
        return handlerInput.responseBuilder
            .speak(
                'I\'m sorry, The Christmas Tree Lights are confused. And not just a bit. I mean, like, totally flummoxed. They have no idea what\'s going on. Perhaps you should say something simpler, that a 4 year old would understand.')
            .getResponse();
    },
};

//////////////////////////////////////////////////////////////////////
// Help

const HelpHandler = {
    canHandle(handlerInput) {
        const request = handlerInput.requestEnvelope.request;
        return request.type === 'IntentRequest' &&
               request.intent.name === 'AMAZON.HelpIntent';
    },
    handle(handlerInput) {
        return handlerInput.responseBuilder
            .speak(
                'You can say things like "tell the christmas tree lights to go red"')
            .getResponse();
    },
};

//////////////////////////////////////////////////////////////////////
// Exit

const ExitHandler = {
    canHandle(handlerInput) {
        const request = handlerInput.requestEnvelope.request;
        return request.type === 'IntentRequest' &&
               (request.intent.name === 'AMAZON.CancelIntent' ||
                request.intent.name === 'AMAZON.StopIntent');
    },
    handle(handlerInput) {
        return handlerInput.responseBuilder.speak('Bye').getResponse();
    },
};

//////////////////////////////////////////////////////////////////////
// SessionEnded

const SessionEndedRequestHandler = {
    canHandle(handlerInput) {
        return handlerInput.requestEnvelope.request.type ===
               'SessionEndedRequest';
    },
    handle(handlerInput) {
        console.log(`Session ended with reason: ${
            handlerInput.requestEnvelope.request.reason}`);
        return handlerInput.responseBuilder.getResponse();
    },
};

//////////////////////////////////////////////////////////////////////
// Error

const ErrorHandler = {
    canHandle() { return true; },
    handle(handlerInput, error) {
        let input_text = JSON.stringify(handlerInput);
        console.log(`Error input: ${input_text}`);
        console.log(`Error handled: ${
            handlerInput.requestEnvelope.request.type} ${
            handlerInput.requestEnvelope.request.type === 'IntentRequest'
                ? `intent: ${handlerInput.requestEnvelope.request.intent.name} `
                : ''}${error.message}.`);
        return handlerInput.responseBuilder
            .speak('Sorry, I can\'t understand the command. Please say again.')
            .reprompt(
                'Sorry, I can\'t understand the command. Please say again.')
            .getResponse();
    },
};

//////////////////////////////////////////////////////////////////////
// Utility functions

//////////////////////////////////////////////////////////////////////
// call service API

async function turn(state) {

    console.log("We're turning!");

    // set this to false to target the spiffy new websocket STUN thing
    const oldserv = false;

    var server = '52.200.210.232'
    var port = 5001;
    var path = ''
    var headers = {'X-DeviceID' : '123'};

    if (oldserv) {
        path = 'set'
        port = 3000;
        headers = null;
    }
    const params = {
        url : `http://${server}:${port}/${path}`,
        json : true,
        headers : headers,
        body : state
    };
    console.log("We're turning: " + JSON.stringify(params));
    try {
        const response = await req.post(params);
        console.log("Did it!");
        return Promise.resolve(response);
    } catch (error) {
        console.log("Error: " + error);
        Promise.reject(error);
    }
}

//////////////////////////////////////////////////////////////////////
// get sanitized slot values

function getSlotValues(filledSlots) {
    const slotValues = {};

    console.log(`The filled slots: ${JSON.stringify(filledSlots)}`);
    Object.keys(filledSlots).forEach((item) => {
        const name = filledSlots[item].name;

        if (filledSlots[item] && filledSlots[item].resolutions &&
            filledSlots[item].resolutions.resolutionsPerAuthority[0] &&
            filledSlots[item].resolutions.resolutionsPerAuthority[0].status &&
            filledSlots[item]
                .resolutions.resolutionsPerAuthority[0]
                .status.code) {
            switch (filledSlots[item]
                        .resolutions.resolutionsPerAuthority[0]
                        .status.code) {
            case 'ER_SUCCESS_MATCH':
                slotValues[name] = {
                    synonym : filledSlots[item].value,
                    resolved : filledSlots[item]
                                   .resolutions.resolutionsPerAuthority[0]
                                   .values[0]
                                   .value.name,
                    isValidated : true,
                };
                break;
            case 'ER_SUCCESS_NO_MATCH':
                slotValues[name] = {
                    synonym : filledSlots[item].value,
                    resolved : filledSlots[item].value,
                    isValidated : false,
                };
                break;
            default:
                break;
            }
        } else {
            slotValues[name] = {
                synonym : filledSlots[item].value,
                resolved : filledSlots[item].value,
                isValidated : false,
            };
        }
    }, this);

    return slotValues;
}

//////////////////////////////////////////////////////////////////////
// SMART HOME discovery

async function handleDiscovery(request, context) {
    let payload = {
        "endpoints" : [ {
            "applianceId" : "54321",
            "endpointId" : "xtl_id",
            "manufacturerName" : "Charlie Skilbeck",
            "friendlyName" : "Christmas Tree Lights",
            "description" : "WiFi Enabled Christmas Tree Lights",
            "displayCategories" : [ "SWITCH" ],
            "cookie" : {
                "key1" :
                    "arbitrary key/value pairs for skill to reference this endpoint.",
                "key2" : "There can be multiple entries",
                "key3" : "but they should only be used for reference purposes.",
                "key4" :
                    "This is not a suitable place to maintain current endpoint state."
            },
            "capabilities" : [
                {
                    "type" : "AlexaInterface",
                    "interface" : "Alexa",
                    "version" : "3"
                },
                {
                    "interface" : "Alexa.PowerController",
                    "type" : "AlexaInterface",
                    "version" : "3",
                    "properties" : {
                        "supported" : [ {"name" : "powerstate"} ],
                        "proactivelyReported" : true
                    },
                },
                {
                    "interface" : "Alexa.ColorController",
                    "type" : "AlexaInterface",
                    "version" : "3",
                    "properties" : {
                        "supported" : [ {"name" : "color"} ],
                        "proactivelyReported" : true
                    }
                },
                {
                    "interface" : "Alexa.BrightnessController",
                    "type" : "AlexaInterface",
                    "version" : "3",
                    "properties" : {
                        "supported" : [ {"name" : "brightness"} ],
                        "proactivelyReported" : true
                    }
                }
            ]
        } ]
    };
    let header = request.directive.header;
    header.name = "Discover.Response";
    console.log("Discovery Response: " +
                JSON.stringify({header : header, payload : payload}));
    context.succeed({event : {header : header, payload : payload}});
}

//////////////////////////////////////////////////////////////////////
// SMART HOME power control

async function handleColorControl(request, context) {

    let namespace =
        request.directive.header
            .namespace; // EG Alexa.PowerController or Alexa.ColorController
    let requestMethod =
        request.directive.header
            .name; // Relates to the namespace, TurnOn, TurnOff, GoSomeColor etc

    let responseHeader = request.directive.header;
    responseHeader.namespace = "Alexa";
    responseHeader.name = "Response";
    responseHeader.messageId = responseHeader.messageId + "-R";

    let requestToken = request.directive.endpoint.scope.token;
    let hsb = request.directive.payload.color;
    let colorResult = rgb_from_hsv(hsb);

    await turn({"powerState" : true, "colors" : [ colorResult ]});

    let contextResult = {
        "properties" : [ {
            "namespace" : "Alexa.ColorController",
            "name" : "color",
            "value" : {
                "hue" : hsb.hue,
                "saturation" : hsb.saturation,
                "brightness" : hsb.brightness
            },
            "timeOfSample" : new Date(),
            "uncertaintyInMilliseconds" : 500
        } ]
    };
    let response = {
        context : contextResult,
        event : {
            header : responseHeader,
            endpoint : {
                scope : {type : "BearerToken", token : requestToken},
                endpointId : "demo_id"
            },
            payload : {}
        }
    };
    console.log("Alexa.ColorController " + JSON.stringify(response));
    context.succeed(response);
}

async function handleBrightness(request, context) {

    let namespace =
        request.directive.header
            .namespace; // EG Alexa.PowerController or Alexa.ColorController
    let requestMethod =
        request.directive.header.name; // Relates to the namespace, TurnOn,
    // TurnOff, AdjustBrightness etc

    let responseHeader = request.directive.header;
    responseHeader.namespace = "Alexa";
    responseHeader.name = "Response";
    responseHeader.messageId = responseHeader.messageId + "-R";

    let p = request.directive.payload;
    console.log("Payload: " + JSON.stringify(p));

    if (requestMethod === 'AdjustBrightness') {
        await turn({
            "powerstate" : true,
            "brightness_delta" : p.brightnessDelta,
        });
    } else if (requestMethod === 'SetBrightness') {
        await turn({
            "powerstate" : true,
            "brightness" : p.brightness,
        });
    }

    let requestToken = request.directive.endpoint.scope.token;

    // send a thing to the server
    // set to single color mode
    // set color 0
    // leave action alone

    let contextResult = {
        "properties" : [ {
            "namespace" : "Alexa.BrightnessController",
            "name" : "brightness",
            "value" : 42,
            "timeOfSample" : new Date(),
            "uncertaintyInMilliseconds" : 500
        } ]
    };
    let response = {
        context : contextResult,
        event : {
            header : responseHeader,
            endpoint : {
                scope : {type : "BearerToken", token : requestToken},
                endpointId : "demo_id"
            },
            payload : {}
        }
    };
    console.log("Alexa.BrightnessController " + JSON.stringify(response));
    context.succeed(response);
}

async function handlePowerControl(request, context) {

    // get device ID passed in during discovery
    let namespace =
        request.directive.header
            .namespace; // EG Alexa.PowerController or Alexa.ColorController
    let requestMethod =
        request.directive.header
            .name; // Relates to the namespace, TurnOn, TurnOff, GoSomeColor etc

    let responseHeader = request.directive.header;
    responseHeader.namespace = "Alexa";
    responseHeader.name = "Response";
    responseHeader.messageId = responseHeader.messageId + "-R";
    // get user token pass in request
    let requestToken = request.directive.endpoint.scope.token;
    let powerResult;

    if (requestMethod === "TurnOn") {
        await turn({"powerstate" : true});
        console.log("DEBUG: Turned it on!");
        powerResult = "ON";
    } else if (requestMethod === "TurnOff") {
        await turn({"powerstate" : false});
        console.log("DEBUG: Turned it off!");
        powerResult = "OFF";
    }
    let contextResult = {
        "properties" : [ {
            "namespace" : "Alexa.PowerController",
            "name" : "powerState",
            "value" : powerResult,
            "timeOfSample" : new Date(),
            "uncertaintyInMilliseconds" : 50
        } ]
    };
    let response = {
        context : contextResult,
        event : {
            header : responseHeader,
            endpoint : {
                scope : {type : "BearerToken", token : requestToken},
                endpointId : "demo_id"
            },
            payload : {}
        }
    };
    console.log("Alexa.PowerController " + JSON.stringify(response));
    context.succeed(response);
}

//////////////////////////////////////////////////////////////////////
// SMART HOME HANDLERS

//////////////////////////////////////////////////////////////////////

const discovery_handler = {
    "namespace" : "Alexa.Discovery",
    "names" : [ "Discover" ],
    "handle" : async function(
        request, context) { await handleDiscovery(request, context); }
};

//////////////////////////////////////////////////////////////////////

const power_handler = {
    "namespace" : "Alexa.PowerController",
    "names" : [ "TurnOn", "TurnOff" ],
    "handle" : async function(
        request, context) { await handlePowerControl(request, context); }
};

//////////////////////////////////////////////////////////////////////

const color_handler = {
    "namespace" : "Alexa.ColorController",
    "names" : [ "SetColor" ],
    "handle" : async function(
        request, context) { await handleColorControl(request, context); }
};

//////////////////////////////////////////////////////////////////////

const brightness_handler = {
    "namespace" : "Alexa.BrightnessController",
    "names" : [ "AdjustBrightness", "SetBrightness" ],
    "handle" : async function(
        request, context) { await handleBrightness(request, context); }
};

//////////////////////////////////////////////////////////////////////

handlers =
    [ discovery_handler, power_handler, color_handler, brightness_handler ];

//////////////////////////////////////////////////////////////////////

async function smart_home_handler(request, context) {
    console.log("smart_home_handler: request = " + JSON.stringify(request));
    if (request.directive && request.directive.header) {
        let header = request.directive.header;
        for (let i = 0; i < handlers.length; ++i) {
            let h = handlers[i];
            if (header.namespace === h.namespace) {
                for (let i = 0; i < h.names.length; ++i) {
                    if (header.name === h.names[i]) {
                        console.log(
                            `SMART HOME HANDLES ${h.namespace}/${h.names[i]}`);
                        await h.handle(request, context);
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

//////////////////////////////////////////////////////////////////////
// LAMBDA SETUP

console.log("Lambda begins");

exports.handler =
    function(request, context, callback) {
    console.log("Here we go");

    let custom_skill_handler =
        Alexa.SkillBuilders.custom()
            .addRequestHandlers(LaunchRequestHandler, FallbackHandler,
                                SolidHandler, SpeedHandler, ActionHandler,
                                HelpHandler, ExitHandler,
                                SessionEndedRequestHandler)
            .addErrorHandlers(ErrorHandler)
            .lambda();

    smart_home_handler(request, context)
        .then(function(response) {
            if (!response) {
                console.log("falling back to custom skill handler");
                return custom_skill_handler(request, context, callback);
            }
            callback(null, response);
        })
        .catch(function(err) { callback(err, null); });
}

    console.log("Lambda done");