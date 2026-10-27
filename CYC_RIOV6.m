% Ciclo da PROVA - Urban Concept
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% CABEÇALHO DE COMPATIBILIDADE COM ADVISOR 2003
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
cyc_description='Ciclo prova completa - largada + 9 voltas tipicas + chegada';
cyc_version=2003; % << NÃO RECONHE SEM
cyc_proprietary=0; 
cyc_validation=0; 
disp(['Data loaded: ',cyc_description])
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% FIM DO CABEÇALHO
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% --- PARÂMETROS ---
v_kmh_max = 31;
v_kmh_min = 16;
v_max_reta  = v_kmh_max * 0.621371; % mph
v_curva     = v_kmh_min * 0.621371; % mph

% velocidade máxima ESPECÍFICA pra fatia 2a (a que termina na parada
% obrigatória) -> pode ser diferente de v_kmh_max, já que não faz sentido
% acelerar até o máximo só pra frear em seguida. AJUSTE conforme estratégia.
v_kmh_max_parada = 24;
v_max_parada = v_kmh_max_parada * 0.621371; % mph

% velocidade máxima ESPECÍFICA pra fatia 2b (saindo da parada, resto da
% Reta 2),também pode ser diferente das outras. AJUSTE conforme estratégia.
v_kmh_max_2b = 31;
v_max_2b = v_kmh_max_2b * 0.621371; % mph

% acelerações que partem do REPOUSO 
% tempo pra Reta 1 na largada (única vez, alvo = v_max_reta)
tempo_acel_inicial_s = 26;
% tempo pra fatia 2b, toda vez que sai de uma parada obrigatória (alvo = v_max_2b)
tempo_acel_2b_s = 13; % AJUSTE independente, já que a velocidade-alvo pode ser diferente

% aceleração que parte de v_curva (saindo de curva), reta INTEIRA (Reta 1
% dentro das voltas típicas, que não tem parada no meio)
tempo_acel_s_reta = 30;

% aceleração que parte de v_curva, mas só a METADE da reta (fatia 2a,
% da curva até o ponto de parada, no meio da Reta 2) AJUSTE depois
% de conferir a distância real dessa fatia
tempo_acel_2a_s = 12;

% desaceleração em curva  separada conforme de onde vem
% vindo da Reta 1 (v_max_reta -> v_curva), usada na largada e na 2ª curva das voltas típicas
tempo_deslizar_curva_reta_s = 44;
% vindo da fatia 2b (v_max_2b -> v_curva), usada na 1ª curva das voltas típicas e na chegada
tempo_deslizar_curva_2b_s = 44; % AJUSTE independente, já que a velocidade de origem pode ser diferente

% frenagem na parada obrigatória (curva/reta até 0)
tempo_frenagem_final_s = 2; % AJUSTE com teste real do carro
tempo_hold_parada_s    = 2;   % tempo parado em cada parada obrigatória
% --- FIM DOS PARÂMETROS ---

NUM_VOLTAS_TIPICAS = 9; 

cyc_time_s   = [0];
temp_spd_vec = [0];

% ===================== LARGADA (1x) =====================
% acelera Reta 1 inteira (do repouso)
cyc_time_s(end+1,1)   = cyc_time_s(end) + tempo_acel_inicial_s;
temp_spd_vec(end+1,1) = v_max_reta;
% curva (vem da Reta 1)
cyc_time_s(end+1,1)   = cyc_time_s(end) + tempo_deslizar_curva_reta_s;
temp_spd_vec(end+1,1) = v_curva;
% acelera fatia 2a (até o ponto de parada)
cyc_time_s(end+1,1)   = cyc_time_s(end) + tempo_acel_2a_s;
temp_spd_vec(end+1,1) = v_max_parada;
% freio real até 0
cyc_time_s(end+1,1)   = cyc_time_s(end) + tempo_frenagem_final_s;
temp_spd_vec(end+1,1) = 0;
% segura parado (PARADA OBRIGATÓRIA #1)
cyc_time_s(end+1,1)   = cyc_time_s(end) + tempo_hold_parada_s;
temp_spd_vec(end+1,1) = 0;

% ===================== VOLTAS TÍPICAS (9x) =====================
for volta = 1:NUM_VOLTAS_TIPICAS
    % acelera fatia 2b (resto da Reta 2, saindo do repouso da parada anterior)
    cyc_time_s(end+1,1)   = cyc_time_s(end) + tempo_acel_2b_s;
    temp_spd_vec(end+1,1) = v_max_2b;
    % curva (vem da fatia 2b)
    cyc_time_s(end+1,1)   = cyc_time_s(end) + tempo_deslizar_curva_2b_s;
    temp_spd_vec(end+1,1) = v_curva;
    % acelera Reta 1 inteira
    cyc_time_s(end+1,1)   = cyc_time_s(end) + tempo_acel_s_reta;
    temp_spd_vec(end+1,1) = v_max_reta;
    % curva (vem da Reta 1)
    cyc_time_s(end+1,1)   = cyc_time_s(end) + tempo_deslizar_curva_reta_s;
    temp_spd_vec(end+1,1) = v_curva;
    % acelera fatia 2a (até o ponto de parada)
    cyc_time_s(end+1,1)   = cyc_time_s(end) + tempo_acel_2a_s;
    temp_spd_vec(end+1,1) = v_max_parada;
    % freio real até 0
    cyc_time_s(end+1,1)   = cyc_time_s(end) + tempo_frenagem_final_s;
    temp_spd_vec(end+1,1) = 0;
    % segura parado (PRÓXIMA PARADA OBRIGATÓRIA)
    cyc_time_s(end+1,1)   = cyc_time_s(end) + tempo_hold_parada_s;
    temp_spd_vec(end+1,1) = 0;
end

% ===================== CHEGADA (1x) =====================
% acelera fatia 2b (resto da Reta 2, saindo da última parada obrigatória)
cyc_time_s(end+1,1)   = cyc_time_s(end) + tempo_acel_2b_s;
temp_spd_vec(end+1,1) = v_max_2b;
% curva (vem da fatia 2b)
cyc_time_s(end+1,1)   = cyc_time_s(end) + tempo_deslizar_curva_2b_s;
temp_spd_vec(end+1,1) = v_curva;
% freio real, parando exatamente no ponto de partida (sem passar pela Reta 1)
cyc_time_s(end+1,1)   = cyc_time_s(end) + tempo_frenagem_final_s;
temp_spd_vec(end+1,1) = 0;
% segura parado (fim da prova)
cyc_time_s(end+1,1)   = cyc_time_s(end) + tempo_hold_parada_s;
temp_spd_vec(end+1,1) = 0;

% --- CORREÇÃO DE VARIÁVEIS DO ADVISOR ---
cyc_mph = [cyc_time_s, temp_spd_vec];

% Elevação DESATIVADA .
cyc_grade = [0 0; 1 0];

key_state_vec = ones(size(cyc_time_s));
vc_key_on = [cyc_time_s, key_state_vec];

cyc_avg_time=3;
cyc_cargo_mass=[0 0;1 0];
cyc_elevation_init=1500;
cyc_filter_bool=0;

% --- CHECAGEM REGULAMENTO ---
n_pts = length(cyc_time_s);
dist_acumulada_mi = zeros(n_pts,1);
for i = 2:n_pts
    dt_s = cyc_time_s(i) - cyc_time_s(i-1);
    v_media_mph = (temp_spd_vec(i) + temp_spd_vec(i-1)) / 2;
    dist_acumulada_mi(i) = dist_acumulada_mi(i-1) + v_media_mph * (dt_s/3600);
end

TEMPO_LIMITE_S    = 1500; % 25 min, do regulamento
DISTANCIA_ALVO_M  = 8530; % 10 voltas, do regulamento

tempo_total_s     = cyc_time_s(end);
distancia_total_m = dist_acumulada_mi(end) * 1609.344;

diff_tempo_s = tempo_total_s - TEMPO_LIMITE_S;
diff_dist_m  = distancia_total_m - DISTANCIA_ALVO_M;

fprintf('\n================ CHECAGEM REGULAMENTAR ================\n');
fprintf('TEMPO TOTAL: %.1f s (%.2f min)  |  limite: %.0f s (25 min)\n', tempo_total_s, tempo_total_s/60, TEMPO_LIMITE_S);
if diff_tempo_s > 0
    fprintf('  >> ESTOUROU %.1f s (%.2f min). REDUZIR.\n', diff_tempo_s, diff_tempo_s/60);
else
    fprintf('  >> OK, com %.1f s de folga.\n', -diff_tempo_s);
end

fprintf('DISTANCIA TOTAL: %.1f m (%.3f km)  |  alvo: %.0f m (%.1f km)\n', ...
    distancia_total_m, distancia_total_m/1000, DISTANCIA_ALVO_M, DISTANCIA_ALVO_M/1000);
if abs(diff_dist_m) < 50
    fprintf('  >> diferença distancia de %.1f m).\n', diff_dist_m);
elseif diff_dist_m > 0
    fprintf('  >> ACIMA  %.1f m. reduzir velocidade/tempo.\n', diff_dist_m);
else
    fprintf('  >> ABAIXO m %.1f m. aumentar velocidade/tempo.\n', -diff_dist_m);
end
fprintf('Numero de paradas obrigatorias no ciclo: %d (largada + %d voltas)\n', 1+NUM_VOLTAS_TIPICAS, NUM_VOLTAS_TIPICAS);
fprintf('=========================================================\n\n');