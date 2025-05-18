# Sistema de deteção de moedas

## Descrição geral

Este projeto implementa um sistema de visão por computador para detetar e classificar moedas de euro em fluxos de vídeo. O sistema utiliza várias técnicas de processamento de imagem e de visão por computador para identificar diferentes denominações de moedas e calcular o seu valor total.

## Caraterísticas

### Técnicas de segmentação
- Segmentação de cor HSV**: O sistema utiliza a segmentação por espaço de cor HSV para diferenciar entre moedas de cobre (1, 2, 5 cêntimos) e moedas de ouro/prata (10, 20, 50 cêntimos, 1€, 2€).
- Segmentação baseada no brilho**: O limiar de brilho adicional ajuda a distinguir os tipos de moedas.

### Melhoria da imagem
- Remoção de ruído**: São utilizadas operações morfológicas binárias (abertura) para remover ruído e pequenos artefactos.
- **Filtragem de fundo**: O sistema filtra os fundos brancos/brilhantes para isolar melhor as moedas.

### Análise de imagem
- Cálculo da área**: A área de cada moeda detectada é medida para ajudar na classificação.
- Deteção da caixa delimitadora**: As regiões rectangulares que contêm moedas são identificadas e marcadas.
- Medição da circularidade**: O sistema calcula a circularidade de cada objeto detectado, filtrando os objectos não circulares.
- Centro de massa**: O centroide de cada moeda é calculado e marcado para visualização.

### Classificação de moedas
- Classificação baseada no tamanho**: As moedas são classificadas com base no seu diâmetro.
- Classificação baseada na cor**: Diferentes materiais (cobre, ouro, prata) são detectados através das propriedades da cor.
- Deteção bimetálica**: Processamento especial para moedas de 1€ e 2€, que têm dois metais diferentes.

### Visualização
- Visualização da caixa delimitadora**: Cada moeda é destacada com uma caixa delimitadora colorida.
- Etiquetas de valor**: O valor de cada moeda é apresentado diretamente na imagem.
- Marcadores de centro**: O centro de massa de cada moeda é marcado para referência.
- Informação da área**: A área de cada moeda é mostrada para depuração e análise.

## Configuração do Visual Studio

Se estiver a utilizar o Visual Studio e encontrar erros de "Stack Overflow", siga estes passos:

1. Clique com o botão direito do rato no seu projeto no Explorador de Soluções
2. Selecione "Propriedades"
3. Vá para Propriedades de configuração > Linker (?) > Sistema
4. Defina "Stack Reserve Size" (?) para 8388608 (8MB) ou superior
5. Clique em Aplicar e reconstrua o projeto

Isto é necessário porque os algoritmos de deteção de moedas utilizam temporariamente uma quantidade significativa de memória.

## Dependências

- OpenCV (4.x recomendado)
- CMake (3.21 ou superior)
- Compilador C++ com suporte a C++11