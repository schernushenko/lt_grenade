
#include "timer.h"

uint g_time; //���������� ����� - ����������� ��������

ISR (TIMER0_OVF_vect) {
	g_time++;
}

//��������� ������ �� ���������� ������, ������ ��� ��������� � �-��� �������� �� ��������� ������
void process_timer (struct timer *timer) {
	if (!timer->active) {
		return;
	};

	uint _time = g_time;

	timer->delta += _time - timer->start_time;
	timer->start_time = _time;

	if (timer->delta > timer->limit) {
		timer->active = 0;
		if (timer->fn_done) {
			timer->fn_done();
		};
		return;
	};

	return;
};

//�������� � �������� ������
void start_timer(struct timer *timer, uint _limit, void(*fn)()) {
	timer->delta = 0;
	timer->active = 1;
	timer->start_time = g_time;
	timer->limit = _limit;
	timer->fn_done = fn;
};

void stop_timer(struct timer *timer) {
	timer->active = 0;
};
