/*
 * CP2 - Sistema de Dados Robusto
 * Aluna: Sthefanie Aiko
 * RM: 87493
 */

#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_task_wdt.h"

#define TASK1_OK   (1 << 0)
#define TASK1_FAIL (1 << 1)
#define TASK2_OK   (1 << 2)
#define TASK2_FAIL (1 << 3)

static QueueHandle_t fila = NULL;
static int flag_status = 0;

/* ==============================
 * 1 - MÓDULO DE GERAÇÃO DE DADOS
 * ============================== */
void TaskGeracao(void *pv)
{
    int valor = 0;
    while (1)
    {
        valor++;

        if (xQueueSend(fila, &valor, 0) != pdTRUE)
        {
            flag_status |= TASK1_FAIL;
            printf("{Sthefanie Aiko-RM:87493} [FILA CHEIA] Não foi possível enviar valor %d\n", valor);
        }
        else
        {
            flag_status |= TASK1_OK;
            printf("{Sthefanie Aiko-RM:87493} [FILA] Valor %d enviado com sucesso!\n", valor);
        }

        esp_task_wdt_reset();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* ==============================
 * 2 - MÓDULO DE RECEPÇÃO DE DADOS
 * ============================== */
void TaskRecepcao(void *pv)
{
    int valor;
    int timeout = 0;

    while (1)
    {
        if (xQueueReceive(fila, &valor, 0) != pdTRUE)
        {
            timeout++;

            if (timeout == 10)
                printf("{Sthefanie Aiko-RM:87493} [TIMEOUT] Recuperação leve - aguardando dados\n");
            else if (timeout == 20)
            {
                printf("{Sthefanie Aiko-RM:87493} [TIMEOUT] Recuperação moderada - limpando fila\n");
                xQueueReset(fila);
            }
            else if (timeout == 30)
            {
                flag_status |= TASK2_FAIL;
                printf("{Sthefanie Aiko-RM:87493} [TIMEOUT] Recuperação agressiva - reiniciando sistema\n");
                vTaskDelay(pdMS_TO_TICKS(1000));
                esp_restart();
            }
        }
        else
        {
            timeout = 0;

           
            int *valor_temp = (int *) malloc(sizeof(int));
            if (valor_temp == NULL)
            {
                flag_status |= TASK2_FAIL;
                printf("{Sthefanie Aiko-RM:87493} [ERRO] Falha ao alocar memória para valor recebido\n");
                vTaskDelay(pdMS_TO_TICKS(500));
                continue;
            }

            *valor_temp = valor;
            printf("{Sthefanie Aiko-RM:87493} [RECEPÇÃO] Valor %d recebido e armazenado dinamicamente\n", *valor_temp);
            printf("{Sthefanie Aiko-RM:87493} [TRANSMISSÃO] Valor %d transmitido com sucesso\n", *valor_temp);

            // Libera memória após uso
            free(valor_temp);

            flag_status |= TASK2_OK;
        }

        esp_task_wdt_reset();
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}


/* ==============================
 * 3 - MÓDULO DE SUPERVISÃO
 * ============================== */
void TaskSupervisao(void *pv)
{
    while (1)
    {
        if (flag_status & TASK1_OK)
            printf("{Sthefanie Aiko-RM:87493} [SUPERVISÃO] Task1 (Geração) OK\n");

        if (flag_status & TASK1_FAIL)
            printf("{Sthefanie Aiko-RM:87493} [SUPERVISÃO] Task1 (Geração) FALHOU\n");

        if (flag_status & TASK2_OK)
            printf("{Sthefanie Aiko-RM:87493} [SUPERVISÃO] Task2 (Recepção) OK\n");

        if (flag_status & TASK2_FAIL)
            printf("{Sthefanie Aiko-RM:87493} [SUPERVISÃO] Task2 (Recepção) FALHOU\n");

        flag_status = 0;
        esp_task_wdt_reset();
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

/* ==============================
 * 4 - MÓDULO DE STATUS DO SISTEMA
 * ============================== */
void TaskSistema(void *pv)
{
    while (1)
    {
        printf("{Sthefanie Aiko-RM:87493} [SISTEMA] Operação normal, WDT ativo\n");
        esp_task_wdt_reset();
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}

/* ==============================
 * FUNÇÃO PRINCIPAL
 * ============================== */
void app_main(void)
{
    printf("{Sthefanie Aiko-RM:87493} [INICIALIZAÇÃO] Iniciando Sistema de Dados Robusto...\n");

    esp_task_wdt_config_t wdt = {
        .timeout_ms = 5000,
        .idle_core_mask = (1 << 0) | (1 << 1),
        .trigger_panic = true
    };
    esp_task_wdt_init(&wdt);

    fila = xQueueCreate(5, sizeof(int));

    if (fila == NULL)
    {
        printf("{Sthefanie Aiko-RM:87493} [ERRO] Falha na criação da fila. Reiniciando sistema.\n");
        esp_restart();
    }

    TaskHandle_t tGeracao, tRecepcao, tSupervisao, tSistema;

    xTaskCreate(TaskGeracao, "Geracao", 8192, NULL, 5, &tGeracao);
    xTaskCreate(TaskRecepcao, "Recepcao", 8192, NULL, 5, &tRecepcao);
    xTaskCreate(TaskSupervisao, "Supervisao", 8192, NULL, 5, &tSupervisao);
    xTaskCreate(TaskSistema, "Sistema", 8192, NULL, 5, &tSistema);

    esp_task_wdt_add(tGeracao);
    esp_task_wdt_add(tRecepcao);
    esp_task_wdt_add(tSupervisao);
    esp_task_wdt_add(tSistema);

    printf("{Sthefanie Aiko-RM:87493} [INICIALIZAÇÃO] Todas as tarefas criadas com sucesso.\n");
}
