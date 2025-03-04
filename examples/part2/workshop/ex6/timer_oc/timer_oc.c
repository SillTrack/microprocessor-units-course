/**
 ******************************************************************************
 * \file    timer_oc.c
 * \author  Александр Смирнов
 * \version 1.1.0
 * \date    10.04.2023
 * \brief   Программа на языке C для учебного стенда на базе
 *          STM32F072RBT6 в среде разработки Keil uVision 5.
 *          Подключение библиотек поддержки МК STM32F072RBT6 осуществляется
 *          средствами IDE Keil через менеджер пакетов Run-Time Environment (RTE).
 *          Разработать программу, обеспечивающую управление яркостью
 *          светодиодов D1-D8: яркость плавно увеличивается,
 *          затем плавно уменьшается. Цвет вначале красный, затем зеленый и
 *          в конце синий. Процесс бесконечный.
 *          Программа работает в режиме 0 учебного стенда (S1 = 0, S2 = 0).
 ******************************************************************************
 */

/* Подключение заголовочного файла с макроопределениями всех регистров специальных
   функций МК STM32F072RBT6. */
#include <stm32f0xx.h>

/* Функция инициализации светодиодов D1-D8 и линий управления цветом */
void leds_init(void)
{
    /* Включение тактирования порта A */
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;

    /* Включение тактирования порта C */
    RCC->AHBENR |= RCC_AHBENR_GPIOCEN;

    /* Настройка на вывод линий PA6, PA7, PA8 (RED, GREEN, BLUE) */
    GPIOA->MODER |= (GPIO_MODER_MODER6_0 | GPIO_MODER_MODER7_0 | GPIO_MODER_MODER8_0);

    /* Настройка на вывод линий PC0 - PC7 (D1 - D8) */
    GPIOC->MODER |= GPIO_MODER_MODER0_0 | GPIO_MODER_MODER1_0 | GPIO_MODER_MODER2_0 |
                    GPIO_MODER_MODER3_0 | GPIO_MODER_MODER4_0 | GPIO_MODER_MODER5_0 |
                    GPIO_MODER_MODER6_0 | GPIO_MODER_MODER7_0;
}

/* Перечисление с цветами */
typedef enum color_t { RED = 0x01, GREEN = 0x02, BLUE = 0x04} color_t;

/* Функция включения светодиодов и выбора цвета */
void led_set(uint16_t led, color_t color)
{
    uint32_t tmp_color = 0;
    if (color & RED)
    {
        tmp_color |= (1 << 6);
    }

    if (color & GREEN)
    {
        tmp_color |= (1 << 7);
    }

    if (color & BLUE)
    {
        tmp_color |= (1 << 8);
    }

    /* Записываем в регистр данных порта C новое состояние светодиодов.
       Номер бита соответствует номеру светодиода: бит 0 - D1, бит 1 - D2 и
       так далее */
    GPIOC->ODR = led;
    /* Сброс трех битов управления цветом с помощью маски */
    GPIOA->ODR &= ~(7 << 6);
    /* Включение светодиодов нужного цвета */
    GPIOA->ODR = tmp_color;
}

/* Функция инициализации таймера TIM1 */
void timer_init()
{
    /* Включение тактирования TIM1 */
    RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;

    /* Расчет предделителя частоты и кода переполнения таймера
       (максимальный код таймера).
       Пусть таймер счетчик переключается каждую 1 мкс или 1 МГц,
       тогда при частоте тактирования МК fmcu = 8 МГц, требуется предделитель
       на 8 - 8 МГц / 1 МГц = 8.
       Пусть переполнение происходит каждую 1 мс или 1 кГц,
       тогда код переполнения должен быть 1 МГц / 1 кГц = 1000 */
    /* Предделитель частоты */
    TIM1->PSC = 8 - 1; /* Prescaler = PSC + 1 */

    /* Максимальный код таймера (счет идет от 0 до 999) */
    TIM1->ARR = 1000 - 1;

    /* Начальный код в регистре выходного сравнения */
    TIM1->CCR1 = 20;

    /* Включение прерывания по переполнению */
    TIM1->DIER |= TIM_DIER_UIE;

    /* Включение прерывания по сравнению - канал 1 */
    TIM1->DIER |= TIM_DIER_CC1IE;

    /* Включить таймер */
    TIM1->CR1 |= TIM_CR1_CEN;

    /* Установка приоритета прерывания по переполнению таймера.
       В Cortex-M0 четыре уровня приоритета - 0-3.
       0 - наибольший приоритет, 3 - наименьший. */
    NVIC_SetPriority(TIM1_BRK_UP_TRG_COM_IRQn, 0);

    /* Установка приоритета прерывания по сравнению. */
    NVIC_SetPriority(TIM1_CC_IRQn, 0);

    /* Разрешение прерывания по переполнению */
    NVIC_EnableIRQ(TIM1_BRK_UP_TRG_COM_IRQn);

    /* Разрешение прерывания по сравнению. */
    NVIC_EnableIRQ(TIM1_CC_IRQn);
}

/* Переменная для подсчета количества переполнений */
uint16_t up_cnt = 0;
/* Переменная для сохранения яркости - код в канале выходного сравнения */
uint16_t brightness = 20;
/* Переменная для текущего цвета светодиодов */
color_t color = RED;
/* Переменная для направления изменения яркости. 0 - возрастает, 1 - убывает */
uint16_t direction = 0;

/* Подпрограмма обработчик прерываний по переполнению таймера */
void TIM1_BRK_UP_TRG_COM_IRQHandler(void)
{
    /* Сброс флага вызвавшего прерывание */
    TIM1->SR &= ~TIM_SR_UIF;

    /* Подсчет количества переполнений таймера.
       Между каждым переполнением проходит по 1 мс */
    up_cnt++;

    /* Если прошло больше 20 мс */
    if (up_cnt > 20)
    {
        /* Подсчет заново */
        up_cnt = 0;

        /* Если яркость увеличивается */
        if (direction == 0)
        {
            /* Увеличиваем код */
            brightness += 10;
            /* Если яркость максимальная */
            if (brightness >= 500)
            {
                /* Яркость будет уменьшаться */
                direction = 1;
            }
        }
        else
        {
            /* Уменьшаем код */
            brightness -= 10;
            /* Если яркость минимальная */
            if (brightness <= 20)
            {
                /* Яркость будет увеличиваться */
                direction = 0;

                /* Меняем цвет */
                if (color == RED)
                {
                    color = GREEN;
                }
                else if (color == GREEN)
                {
                    color = BLUE;
                }
                else if (color == BLUE)
                {
                    color = RED;
                }
            }
        }
    }

    /* Включение всех светодиодов */
    led_set(0xFF, color);

    /* Запись в регистр выходного сравнения нового кода */
    TIM1->CCR1 = brightness;
}

/* Подпрограмма обработчик прерываний по выходному сравнению таймера */
void TIM1_CC_IRQHandler(void)
{
    /* Сброс флага вызвавшего прерывание */
    TIM1->SR &= ~TIM_SR_CC1IF;

    /* Выключение всех светодиодов */
    led_set(0, color);
}

/* Функция main - точка входа в программу */
int main(void)
{
    /* Инициализация светодиодов D1-D8 */
    leds_init();
    /* Инициализация таймера TIM1 */
    timer_init();

    /* Бесконечный цикл */
    while (1)
    {
        /* Основные действия происходят в подпрограммах обработчиках
           прерываний.
        */
    }
}
