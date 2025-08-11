//Importando as bibliotecas:
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include "math.h"
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "driver/pulse_cnt.h"
#include "driver/gpio.h"
#include "esp_sleep.h"
#include <ultrasonic.h>
#include "driver/gptimer.h"
#include "driver/ledc.h"
#include "soc/clk_tree_defs.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "mqtt_client.h"



//Definição dos pinos utilizados pelos dispositivos conectados ao ESP-32:
#define MD1 16		// Motor Direito Pino 1
#define MD2 17		// Motor Direito Pino 2
#define ME1 18		// Motor Esquerdo Pino 1
#define ME2 19		// Motor Esquerdo Pino 2
#define EDA 34		// Encoder Direito A (CLK)
#define EDB 35		// Encoder Direito B (DT)
#define EEA 36		// Encoder Esquerdo A (CLK)
#define EEB 39		// Encoder Esquerdo B (DT)
//#define USET 23		// Ultrassom Esquerdo (Trig)
//#define USEE 22		// Ultrassom Esquerdo (Echo)
#define USCT 22		// Ultrassom Central (Trig)
#define USCE 23		// Ultrassom Central (Echo)
//#define USDT 27		// Ultrassom Direito (Trig)
//#define USDE 12		// Ultrassom Direito (Echo)
#define CH 21		// Chutador

//Parâmetros de hardware do robô:
#define D 67		//diâmetro da roda do robô, em milímetros
#define R 95		//raio de giro do robô, em milímetros
#define PPR 30		//pulsos por revolução (resolução do encoder) em borda de subida e descida (resolução máxima dobrada)
#define N 280		//Velocidade nominal do motor, em RPM

//Constantes do controlador:
#define kp 1		//Ganho da ação proporcional
#define ki 0		//Ganho da ação integral

//Outras constantes:
#define ts 1		//tempo de amostragem, em ms
#define kf 500		//constante do filtro passa-baixas digital
#define DMIN 80		//Distância mínima permitida para sistema de anti-colisão, em mm



//Variáveis globais:
esp_mqtt_client_handle_t client = NULL;
esp_timer_handle_t kick_timer;
pcnt_unit_handle_t *ED, *EE;
ultrasonic_sensor_t *USD, *USC, *USE;
char topic[15], data[10];
bool kick_state=false, free_front=true, kicking=false, remote_control=false;
float refd=0, refe=0, sad=0, sd=0, vd=0, sae=0, se=0, ve=0, ed=0, ied=0, ud=0, ee=0, iee=0, ue=0, dd=300, dc=300, de=300;
uint64_t t=0, tad=0, tae=0;



//Struct para dados do motor:
typedef struct
{
    int motor_pin_1;
    int motor_pin_2;
    ledc_channel_t pwm_channel;
} motor_t;

//Definição dos motores:
motor_t MD = {
	.motor_pin_1 = MD1,
	.motor_pin_2 = MD2,
	.pwm_channel = LEDC_CHANNEL_0
};
motor_t ME = {
	.motor_pin_1 = ME1,
	.motor_pin_2 = ME2,
	.pwm_channel = LEDC_CHANNEL_1
};





//Função para saturação do sinal:
float sat(float num, int lim_inf, int lim_sup)
{
	if(num > lim_sup)
	{
		return lim_sup;
	}
	if(num < lim_inf)
	{
		return lim_inf;
	}
	return num;
}

//Função de callback para desarmar o chutador:
void kick_callback(void* arg)
{
	if(kick_state)
	{
		gpio_set_level(CH, 0);
		kick_state = false;
		esp_timer_start_once(kick_timer, 5e6);
	}
}

//Função para atualizar estado do chutador:
void kick(int power)
{
	//Satura o valor dentro de um intervalo percentual:
	if(power > 100)
	{
		power = 100;
	}
	//Verifica se há força de chute e se passou-se o tempo mínimo de carregamento:
	kicking = esp_timer_is_active(kick_timer);
	if(power && !kicking)
	{
		//Aciona a solenoide e inicia a contagem de tempo para desativar o atuador:
		gpio_set_level(CH, 1);
		kick_state = true;
		esp_timer_start_once(kick_timer, round((float)500*1e3*(float)power/100.));
	}
}

//Função para configurar e iniciar o wifi:
void wifi()
{
	//Inicializando wifi:
	nvs_flash_init();
	esp_netif_init();
	esp_event_loop_create_default();
	esp_netif_create_default_wifi_sta();
	wifi_init_config_t wifi_initiation = WIFI_INIT_CONFIG_DEFAULT();
	esp_wifi_init(&wifi_initiation);
	wifi_config_t wifi_configuration = {
		.sta = {
					.ssid 		= "robotica",
					.password 	= "renato#305",
		},
	};
	esp_wifi_set_mode(WIFI_MODE_STA);
	esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_configuration);
	esp_wifi_start();
	esp_wifi_connect();
}

//Função de callback do MQTT:
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
	esp_mqtt_event_handle_t event = event_data;
	sprintf(topic, "%.*s", event->topic_len, event->topic);
	sprintf(data, "%.*s", event->data_len, event->data);
	//printf("\n%s\t%s", topic, data);
	if(!strcmp(topic, "pcpy-refd"))
	{
		float val = atof(data);
		if(free_front || (val<0))
		{
			refd = val;
		}
	}
	if(!strcmp(topic, "pcpy-refe"))
	{
		float val = atof(data);
		if(free_front || (val<0))
		{
			refe = val;
		}
	}
	if(!strcmp(topic, "pcpy-kick"))
	{
		//printf("\nMandou chutar!");
		int val = atoi(data);
		kick(val);
	}
	if(!strcmp(topic, "pcpy-rc"))
	{
		bool val = (bool)atoi(data);
		remote_control = val;
		if(remote_control)
		{
			printf("\nREMOTE CONTROL ON!");
			printf("\nREMOTE CONTROL OFF!");
		}
	}
}

//Função para configurar e iniciar o serviço MQTT:
void mqtt()
{
	//Conexão com o broker e ID do dispositivo:
	esp_event_loop_create_default();
	esp_mqtt_client_config_t mqtt_cfg = {
		.broker.address.uri = "mqtt://test.mosquitto.org:1883",
		.credentials.client_id	= "ESP32",
	};
	client = esp_mqtt_client_init(&mqtt_cfg);

	//Configurando a task que é acionada sempre que receber mensagem:
	esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);

	//Iniciando o serviço MQTT:
	esp_mqtt_client_start(client);

	//Subscrevendo nos tópicos:
	esp_mqtt_client_subscribe(client, "pcpy-rc", 0);
	esp_mqtt_client_subscribe(client, "pcpy-refd", 0);
	esp_mqtt_client_subscribe(client, "pcpy-refe", 0);
	esp_mqtt_client_subscribe(client, "pcpy-kick", 0);
}

//Função para configurar o PWM:
void pwm()
{
	//Configuração do timer para geração do PWM:
	ledc_timer_config_t pwmc = {
		.speed_mode 		= LEDC_LOW_SPEED_MODE,
		.timer_num 			= LEDC_TIMER_0,
		.duty_resolution 	= LEDC_TIMER_12_BIT,
		.freq_hz 			= 10e3,
		.clk_cfg 			= LEDC_AUTO_CLK
	};
	ledc_timer_config(&pwmc);
}

//Função para enviar o PWM aos motores:
void set_PWM(motor_t motor, float sig)
{
	gpio_set_direction(motor.motor_pin_1, GPIO_MODE_OUTPUT);
	gpio_set_direction(motor.motor_pin_2, GPIO_MODE_OUTPUT);
	if(sig>=0)
	{
		gpio_set_level(motor.motor_pin_1, 0);
		ledc_channel_config_t pwmch = {
			.speed_mode = LEDC_LOW_SPEED_MODE,
			.channel 	= motor.pwm_channel,
			.timer_sel 	= LEDC_TIMER_0,
			.intr_type 	= LEDC_INTR_DISABLE,
			.gpio_num 	= motor.motor_pin_2,
			.duty		= abs((int)(sig*4095.0/100.0)),
			.hpoint		= 0,
		};
		ledc_channel_config(&pwmch);
	}else
	{
		gpio_set_level(motor.motor_pin_2, 0);
		ledc_channel_config_t pwmch = {
			.speed_mode = LEDC_LOW_SPEED_MODE,
			.channel 	= motor.pwm_channel,
			.timer_sel 	= LEDC_TIMER_0,
			.intr_type 	= LEDC_INTR_DISABLE,
			.gpio_num 	= motor.motor_pin_1,
			.duty		= abs((int)(sig*4095.0/100.0)),
			.hpoint		= 0,
		};
		ledc_channel_config(&pwmch);
	}
}

//Função para definicão do encoder e configuração do módulo contador de pulsos:
pcnt_unit_handle_t *enc_attach(int PIN_A, int PIN_B)
{
	//Alocando a unidade contadora de pulsos:
	pcnt_unit_handle_t *pcnt_unit = (pcnt_unit_handle_t *) malloc(sizeof(pcnt_unit_handle_t));
	pcnt_unit_config_t unit_config = {
	        .high_limit = 1e4,
	        .low_limit = -1e4,
	};
	pcnt_new_unit(&unit_config, pcnt_unit);

	//Filtro para ruído:
	pcnt_glitch_filter_config_t filter_config = {
	    .max_glitch_ns = 1000,
	};
	pcnt_unit_set_glitch_filter(*pcnt_unit, &filter_config);

	//Declaração dos canais para as interrupções:
	pcnt_chan_config_t chan_a_config = {
	    .edge_gpio_num = PIN_A,
	    .level_gpio_num = PIN_B,
	};
	pcnt_channel_handle_t pcnt_chan_a = NULL;
	pcnt_new_channel(*pcnt_unit, &chan_a_config, &pcnt_chan_a);
	pcnt_chan_config_t chan_b_config = {
	    .edge_gpio_num = PIN_B,
	    .level_gpio_num = PIN_A,
	};
	pcnt_channel_handle_t pcnt_chan_b = NULL;
	pcnt_new_channel(*pcnt_unit, &chan_b_config, &pcnt_chan_b);

	//Configuração dos comportamentos de callback nas interrupções:
	pcnt_channel_set_edge_action(pcnt_chan_a, PCNT_CHANNEL_EDGE_ACTION_DECREASE, PCNT_CHANNEL_EDGE_ACTION_INCREASE);
	pcnt_channel_set_level_action(pcnt_chan_a, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE);
	pcnt_channel_set_edge_action(pcnt_chan_b, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_DECREASE);
	pcnt_channel_set_level_action(pcnt_chan_b, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE);

	//Definido a função de callback para estimação das posições:
	//pcnt_unit_register_event_callbacks(*pcnt_unit, cbs, NULL);

	//Inicando a contagem de pulsos:
	pcnt_unit_enable(*pcnt_unit);
	pcnt_unit_clear_count(*pcnt_unit);
	pcnt_unit_start(*pcnt_unit);

	return pcnt_unit;
}

//Função para definição do ultrassom:
ultrasonic_sensor_t *us_attach(int TRIG, int ECHO)
{
	//Alocando o sensor ultrassom:
	ultrasonic_sensor_t *us = (ultrasonic_sensor_t *) malloc(sizeof(ultrasonic_sensor_t));
	ultrasonic_sensor_t pins = {
	    .trigger_pin = TRIG,
	    .echo_pin = ECHO
	};
	*us = pins;

	//Iniciando o sensor ultrassom:
	ultrasonic_init(us);

	return us;
}

//Função para leitura de encoder:
float enc_read(pcnt_unit_handle_t *pcnt_unit)
{
	int pulse_count = 0;
	pcnt_unit_get_count(*pcnt_unit, &pulse_count);
	return (((float)pulse_count/(2.0*PPR))*M_PI*D);
}

//Função para leitura do ultrassom:
int us_read(ultrasonic_sensor_t *us)
{
	float dist=0;
	int dmax = 300;	//distância máxima de leitura do ultrassom, em mm
	if(ultrasonic_measure(us, dmax*1e-3, &dist) == ESP_ERR_ULTRASONIC_ECHO_TIMEOUT)
	{
		return dmax;
	}

	return round(dist*1e3);
}

//Função para reset da contagem de pulsos:
void reset_count(pcnt_unit_handle_t *pcnt_unit)
{
	pcnt_unit_clear_count(*pcnt_unit);
}

//Função para atualizar posição e velocidade:
static void update_data(void *arg)
{
	//Atualizando o tempo atual:
	t = esp_timer_get_time();
	//Posições atuais:
	sd += ((enc_read(ED)-sd)/(float)kf);
	se += ((enc_read(EE)-se)/(float)kf);
	if(sd!=sad)
	{
		float current_v = ((float)(sd-sad)/(float)((t-tad)*1e-6));
		vd += ((current_v-vd)/kf);
		//printf("\nt:%llu,vd:%f", t,vd);
		tad = t;
		sad = sd;
	}
	if(se!=sae)
	{
		float current_v = ((float)(se-sae)/(float)((t-tae)*1e-6));
		ve += ((current_v-ve)/kf);
		tae = t;
		sae = se;
	}
	if((t-tad)>1e6)
	{
		vd = 0;
		tad = t;
		sad = sd;
	}
	if((t-tae)>1e6)
	{
		ve = 0;
		tae = t;
		sae = se;
	}
}

static void check_collision(void *arg)
{
	//dd += ((float)(us_read(USD)-dd)/(float)5.0);
	dc += ((float)(us_read(USC)-dc)/(float)5.0);
	//de += ((float)(us_read(USE)-de)/(float)5.0);
	if((dd<DMIN) || (dc<DMIN) || (de<DMIN))
	{
		//printf("\n Objeto identificado à frente do robô!");
		free_front = false;
		refd = 0;
		refe = 0;
		ied = 0;
		iee = 0;
		vd = 0;
		ve = 0;
	}else
	{
		free_front = true;
	}
}

void right_control()
{
	ed = refd-((float)100.0*vd/((float)M_PI*D*N/60.0));
	if(((ud>-100) || (ed>0)) && ((ud<100) || (ed<0)))
	{
		ied += ed*ts*1e-3;
	}
	ud = ((kp*ed) + (ki*ied));
	ud = sat(ud, -100, 100);
	set_PWM(MD, ud);
}

void left_control()
{
	ee = refe-((float)100.0*ve/((float)M_PI*D*N/60.0));
	if(((ue>-100) || (ee>0)) && ((ue<100) || (ee<0)))
	{
		iee += ee*ts*1e-3;
	}
	ue = ((kp*ee) + (ki*iee));
	ue = sat(ue, -100, 100);
	set_PWM(ME, ue);
}

//Função de callback para o timer de controle dedicado:
static void control(void* arg)
{
	right_control();
	left_control();
}

//Função para publicar os dados nos tópicos:
static void send_data(void *arg)
{
	if(remote_control)
	{
		char send[10];
		//Estado do chutador:
		/*sprintf(send, "%d", ((int)kicking));
		esp_mqtt_client_publish(client, "esp32-kstate", send, 0, 1, 0);
		//Estado do sistema anti-colisão (collision avoidance):
		sprintf(send, "%d", ((int)free_front));
		esp_mqtt_client_publish(client, "esp32-ca", send, 0, 1, 0);
		//Distância ultrassom direito:
		sprintf(send, "%d", (int)round(dd));
		esp_mqtt_client_publish(client, "esp32-dd", send, 0, 1, 0);
		//Distância ultrassom central:
		sprintf(send, "%d", (int)round(dc));
		esp_mqtt_client_publish(client, "esp32-dc", send, 0, 1, 0);
		//Distância ultrassom esquerdo:
		sprintf(send, "%d", (int)round(de));
		esp_mqtt_client_publish(client, "esp32-de", send, 0, 1, 0);
		//Referência motor direito:
		sprintf(send, "%.2f", refd);
		esp_mqtt_client_publish(client, "esp32-refd", send, 0, 1, 0);
		//Referência motor esquerdo:
		sprintf(send, "%.2f", refe);
		esp_mqtt_client_publish(client, "esp32-refe", send, 0, 1, 0);*/
		//Posição roda direita:
		sprintf(send, "%d", (int)round(sd));
		esp_mqtt_client_publish(client, "esp32-sd", send, 0, 0, 0);
		//Posição roda esquerda:
		sprintf(send, "%d", (int)round(se));
		esp_mqtt_client_publish(client, "esp32-se", send, 0, 0, 0);
		//Velocidade roda direita:
		/*sprintf(send, "%.2f", vd);
		esp_mqtt_client_publish(client, "esp32-vd", send, 0, 1, 0);
		//Velocidade roda esquerda:
		sprintf(send, "%.2f", ve);
		esp_mqtt_client_publish(client, "esp32-ve", send, 0, 1, 0);
		//Erro de velocidade roda direita:
		sprintf(send, "%.2f", ed);
		esp_mqtt_client_publish(client, "esp32-ed", send, 0, 1, 0);
		//Erro de velocidade roda esquerda:
		sprintf(send, "%.2f", ee);
		esp_mqtt_client_publish(client, "esp32-ee", send, 0, 1, 0);
		//Sinal de controle motor direito:
		sprintf(send, "%.2f", ud);
		esp_mqtt_client_publish(client, "esp32-ud", send, 0, 1, 0);
		//Sinal de controle motor esquerdo:
		sprintf(send, "%.2f", ue);
		esp_mqtt_client_publish(client, "esp32-ue", send, 0, 1, 0);*/
	}
}

//Função para configurar o timer dedicado:
void timers()
{
	//timer peridódico para atualização da velocidade:
	esp_timer_handle_t velocity_timer;
	const esp_timer_create_args_t velocity_timer_args = {
		.callback = &update_data,
	};
	esp_timer_create(&velocity_timer_args, &velocity_timer);
	esp_timer_start_periodic(velocity_timer, 1e2);

	//timer peridódico para sistema anti-colisão:
	esp_timer_handle_t ca_timer;
	const esp_timer_create_args_t ca_timer_args = {
		.callback = &check_collision,
	};
	esp_timer_create(&ca_timer_args, &ca_timer);
	esp_timer_start_periodic(ca_timer, 10e3);

	//timer peridódico para controle:
	esp_timer_handle_t control_timer;
	const esp_timer_create_args_t control_timer_args = {
		.callback = &control,
	};
	esp_timer_create(&control_timer_args, &control_timer);
	esp_timer_start_periodic(control_timer, (ts*1e3));

	//timer peridódico para envio dos dados:
	esp_timer_handle_t send_data_timer;
	const esp_timer_create_args_t send_data_timer_args = {
		.callback = &send_data,
	};
	esp_timer_create(&send_data_timer_args, &send_data_timer);
	esp_timer_start_periodic(send_data_timer, 1e3);

	//Timer one-shot do chutador:
	const esp_timer_create_args_t one_shot_timer_args = {
		.callback = &kick_callback,
	};
	esp_timer_create(&one_shot_timer_args, &kick_timer);
}





void app_main()
{
	//Configuração e inicialização so wifi:
	wifi();

	sleep(3);

	//Configuração e inicialização do serviço MQTT:
	mqtt();

	//Configuração do PWM:
	pwm();

	//Configuração da GPIO do chutador:
	gpio_config_t gpioc =
	{
	    .pin_bit_mask	= (1ULL << CH),
		.mode			= GPIO_MODE_OUTPUT,
		.pull_up_en		= GPIO_PULLUP_DISABLE,
		.pull_down_en 	= GPIO_PULLDOWN_DISABLE,
	    .intr_type 		= GPIO_INTR_DISABLE,
	};
	gpio_config(&gpioc);

	//Definição dos encoders e configuração dos contadores de pulsos por interrupção:
	ED = enc_attach(EDB, EDA);
	EE = enc_attach(EEA, EEB);

	//Definição dos ultrassons:
	//USD = us_attach(USDT, USDE);
	USC = us_attach(USCT, USCE);
	//USE = us_attach(USET, USEE);

	//Configuração e inicialização dos timers dedicados:
	timers();

	//Serial plot:
	while(1)
	{
		printf("sd:%f,refd:%f,vd:%f,ed:%f,ud:%f\n", sd, ((float)N/60.0*M_PI*D*refd/100.0), vd, ed, ud);
		//printf("se:%f,refe:%f,ve:%f,ee:%f,ue:%f\n", se, ((float)N/60.0*M_PI*D*refe/100.0), ve, ee, ue);
		//printf("de:%f,dc:%f,dd:%f\n", de, dc, dd);
		//kick(100);
		//printf("refd:%f,refe:%f,dc:%f\n", refd, refe, dc);
		vTaskDelay(pdMS_TO_TICKS(10));
	}
}
