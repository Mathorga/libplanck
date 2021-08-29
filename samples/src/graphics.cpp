#include <imago/imago.h>
#include <SFML/Graphics.hpp>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

float randomFloat(float min, float max) {
    float random = ((float)rand()) / (float)RAND_MAX;

    float range = max - min;
    return (random * range) + min;
}

void initPositions(corticolumn column, float* xNeuronPositions, float* yNeuronPositions) {
    for (uint32_t i = 0; i < column.neurons_count; i++) {
        xNeuronPositions[i] = randomFloat(0, 1);
        yNeuronPositions[i] = randomFloat(0, 1);
    }
}

void drawNeurons(corticolumn column, sf::RenderWindow* window, sf::VideoMode videoMode, float* xNeuronPositions, float* yNeuronPositions) {
    for (uint32_t i = 0; i < column.neurons_count; i++) {
        sf::CircleShape neuronSpot;

        neuron* currentNeuron = &(column.neurons[i]);

        float neuronValue = ((float) currentNeuron->value) / ((float) currentNeuron->threshold);

        float radius = 5.0f;
        neuronSpot.setRadius(radius);

        // neuronSpot.setFillColor(sf::Color((sf::Uint8) neuronValue, (sf::Uint8) neuronValue, (sf::Uint8) neuronValue, (sf::Uint8) neuronValue));
        if (currentNeuron->value < 0) {
            neuronSpot.setFillColor(sf::Color::White);
        } else {
            neuronSpot.setFillColor(sf::Color(0, 127, 255, 31 + 224 * neuronValue));
        }
        
        neuronSpot.setPosition(xNeuronPositions[i] * videoMode.width, yNeuronPositions[i] * videoMode.height);

        // Center the spot.
        neuronSpot.setOrigin(radius, radius);
        window->draw(neuronSpot);
    }
}

void drawSynapses(corticolumn column, sf::RenderWindow* window, sf::VideoMode videoMode, float* xNeuronPositions, float* yNeuronPositions) {
    for (uint32_t i = 0; i < column.synapses_count; i++) {
        sf::Vertex line[] = {
            sf::Vertex(
                {xNeuronPositions[column.synapses[i].input_neuron] * videoMode.width, yNeuronPositions[column.synapses[i].input_neuron] * videoMode.height},
                sf::Color(255, 127, 31, 15)),
            sf::Vertex(
                {xNeuronPositions[column.synapses[i].output_neuron] * videoMode.width, yNeuronPositions[column.synapses[i].output_neuron] * videoMode.height},
                sf::Color(31, 127, 255, 15))
        };

        window->draw(line, 2, sf::Lines);
    }
}

void drawSpikes(corticolumn column, sf::RenderWindow* window, sf::VideoMode videoMode, float* xNeuronPositions, float* yNeuronPositions) {
    for (uint32_t i = 0; i < column.spikes_count; i++) {
        sf::CircleShape spikeSpot;
        float radius = 2.0f;
        spikeSpot.setRadius(radius);
        spikeSpot.setFillColor(sf::Color(255, 255, 255, 31));

        synapse* referenceSynapse = &(column.synapses[column.spikes[i].synapse]);
        uint32_t startingNeuron = referenceSynapse->input_neuron;
        uint32_t endingNeuron = referenceSynapse->output_neuron;

        float xDist = xNeuronPositions[endingNeuron] * videoMode.width - xNeuronPositions[startingNeuron] * videoMode.width;
        float yDist = yNeuronPositions[endingNeuron] * videoMode.height - yNeuronPositions[startingNeuron] * videoMode.height;

        float progress = ((float) column.spikes[i].progress) / ((float) referenceSynapse->propagation_time);

        float xSpikePosition = xNeuronPositions[startingNeuron] * videoMode.width + (progress * (xDist));
        float ySpikePosition = yNeuronPositions[startingNeuron] * videoMode.height + (progress * (yDist));
        
        spikeSpot.setPosition(xSpikePosition, ySpikePosition);

        // Center the spot.
        spikeSpot.setOrigin(radius, radius);
        window->draw(spikeSpot);
    }
}

int main(int argc, char **argv) {
    int neuronsCount = 100;
    int synapsesDensity = 10;

    // Input handling.
    switch (argc) {
        case 2:
            neuronsCount = atoi(argv[1]);
            break;
        case 3:
            neuronsCount = atoi(argv[1]);
            synapsesDensity = atoi(argv[2]);
            break;
        default:
            break;
    }

    sf::VideoMode desktopMode = sf::VideoMode::getDesktopMode();


    srand(time(0));

    // Create network model.
    corticolumn column;
    dccol_init(&column, neuronsCount, synapsesDensity);

    float* xNeuronPositions = (float*) malloc(column.neurons_count * sizeof(float));
    float* yNeuronPositions = (float*) malloc(column.neurons_count * sizeof(float));

    initPositions(column, xNeuronPositions, yNeuronPositions);
    
    sf::ContextSettings settings;
    // settings.antialiasingLevel = 16;

    // create the window
    sf::RenderWindow window(desktopMode, "Imago", sf::Style::Fullscreen, settings);
    
    bool feeding = false;

    int counter = 0;

    // Run the program as long as the window is open.
    while (window.isOpen()) {
        usleep(1000);
        counter++;

        // Check all the window's events that were triggered since the last iteration of the loop.
        sf::Event event;
        while (window.pollEvent(event)) {
            switch (event.type) {
                case sf::Event::Closed:
                    // Close requested event: close the window.
                    window.close();
                    break;
                case sf::Event::KeyReleased:
                    switch (event.key.code) {
                        case sf::Keyboard::R:
                            initPositions(column, xNeuronPositions, yNeuronPositions);
                            break;
                        case sf::Keyboard::Escape:
                        case sf::Keyboard::Q:
                            window.close();
                            break;
                        case sf::Keyboard::Space:
                            feeding = !feeding;
                        default:
                            break;
                    }
                    break;
                default:
                    break;
            }
        }

        // Clear the window with black color
        window.clear(sf::Color(31, 31, 31, 255));

        if (counter % 10 == 0){// && feeding) {
            ccol_evolve(&column);
        }

        // Feed the column and tick it.
        uint32_t input_neurons[] = {0, 1, 2, 3};
        if (feeding && randomFloat(0, 1) < 0.4f) {
            ccol_feed(&column, input_neurons, 4, DEFAULT_VALUE);
        }
        ccol_tick(&column);

        // Highlight input neurons.
        for (uint32_t i = 0; i < 4; i++) {
            sf::CircleShape neuronCircle;

            float radius = 7.0f;
            neuronCircle.setRadius(radius);
            neuronCircle.setOutlineThickness(2);
            neuronCircle.setOutlineColor(sf::Color::White);

            neuronCircle.setFillColor(sf::Color::Transparent);
            
            neuronCircle.setPosition(xNeuronPositions[i] * desktopMode.width, yNeuronPositions[i] * desktopMode.height);

            // Center the spot.
            neuronCircle.setOrigin(radius, radius);
            window.draw(neuronCircle);
        }

        // Draw neurons.
        drawNeurons(column, &window, desktopMode, xNeuronPositions, yNeuronPositions);

        // Draw synapses.
        drawSynapses(column, &window, desktopMode, xNeuronPositions, yNeuronPositions);

        // Draw spikes.
        drawSpikes(column, &window, desktopMode, xNeuronPositions, yNeuronPositions);

        // Draw spikes count.
        sf::Font font;
        if (!font.loadFromFile("res/JetBrainsMono.ttf")) {
            printf("Font not loaded\n");
        }
        sf::Text infoText;
        infoText.setPosition(20.0f, 20.0f);
        infoText.setString(
            "Spikes count: " + std::to_string(column.spikes_count) + "\n" +
            "Feeding: " + (feeding ? "true" : "false") + "\n" +
            "Iterations: " + std::to_string(counter) + "\n");
        infoText.setFont(font);
        infoText.setCharacterSize(24);
        infoText.setFillColor(sf::Color::White);
        window.draw(infoText);

        // End the current frame.
        window.display();
    }

    return 0;
}
