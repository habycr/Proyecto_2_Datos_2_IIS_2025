using System;
using System.Linq;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using UI.Models;
using UI.Services;

namespace UI
{
    public partial class MainWindow : Window
    {
        private ProblemApiClient _apiClient;
        private ProblemDto? _currentProblem;
        private bool _isLoadingProblems = false;
        private readonly AnalyzerApiClient _analyzerService;
        private bool _serverConnected = false;
        private object selectedProblem;





        // Flag para habilitar/deshabilitar compilación (cambiar a true cuando esté lista)
        private const bool COMPILATION_ENABLED = false;

        public MainWindow()
        {
            InitializeComponent();
            _apiClient = new ProblemApiClient("http://localhost:8080");
            _analyzerService = new AnalyzerApiClient();
            Loaded += MainWindow_Loaded;
            CheckServerConnection();
        }

        // ==================== Evento: Ventana cargada ====================
        private async void MainWindow_Loaded(object sender, RoutedEventArgs e)
        {
            await LoadProblemListAsync();
        }

        // ==================== Cargar lista de problemas ====================
        private async System.Threading.Tasks.Task LoadProblemListAsync()
        {
            if (_isLoadingProblems) return;

            try
            {
                _isLoadingProblems = true;
                DisableButtons();

                ConsoleOutput.Text = " Conectando al Gestor de Problemas...\n";

                bool isHealthy = await _apiClient.IsHealthyAsync();
                if (!isHealthy)
                {
                    ConsoleOutput.Text += " ERROR: No se puede conectar al servidor en http://localhost:8080\n";
                    ConsoleOutput.Text += "   → Asegúrate de que 'problem_manager_api.exe' esté corriendo.\n";
                    ConsoleOutput.Text += "   → Verifica que MongoDB esté activo.\n";
                    return;
                }

                ConsoleOutput.Text += " Conexión exitosa al servidor.\n";

                var problems = await _apiClient.GetAllProblemsAsync();

                if (problems.Count == 0)
                {
                    ConsoleOutput.Text += "\n No hay problemas en la base de datos.\n";
                    ConsoleOutput.Text += "   → Puedes crear algunos usando el Modo Administrador.\n";
                    ProblemList.ItemsSource = null;
                    return;
                }

                ProblemList.ItemsSource = problems;
                ProblemList.DisplayMemberPath = "title";

                ConsoleOutput.Text += $" {problems.Count} problema(s) cargado(s) correctamente.\n\n";
                ConsoleOutput.Text += " Selecciona un problema de la lista para ver sus detalles.\n";
            }
            catch (Exception ex)
            {
                ConsoleOutput.Text += $" ERROR: {ex.Message}\n";
            }
            finally
            {
                _isLoadingProblems = false;
                EnableButtons();
            }
        }

        // ==================== Filtro por Dificultad ====================
        private async void DifficultyComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            // Evitar ejecución durante la inicialización de la ventana
            if (!this.IsLoaded) return;
            if (DifficultyComboBox.SelectedItem == null) return;
            if (_isLoadingProblems) return;

            string selectedDifficulty = ((ComboBoxItem)DifficultyComboBox.SelectedItem).Content.ToString();

            if (selectedDifficulty == "Todas")
            {
                await LoadProblemListAsync();
                return;
            }

            try
            {
                _isLoadingProblems = true;
                DisableButtons();

                ConsoleOutput.Text = $" Filtrando por dificultad: {selectedDifficulty}...\n";

                var problems = await _apiClient.GetProblemsByDifficultyAsync(selectedDifficulty);

                if (problems.Count == 0)
                {
                    ConsoleOutput.Text += $" No hay problemas con dificultad '{selectedDifficulty}'.\n";
                    ProblemList.ItemsSource = null;
                    return;
                }

                ProblemList.ItemsSource = problems;
                ProblemList.DisplayMemberPath = "title";

                ConsoleOutput.Text += $" {problems.Count} problema(s) encontrado(s).\n";
            }
            catch (Exception ex)
            {
                ConsoleOutput.Text += $" ERROR al filtrar: {ex.Message}\n";
            }
            finally
            {
                _isLoadingProblems = false;
                EnableButtons();
            }
        }

        // ==================== Filtro por Tag ====================
        private async void FilterByTagButton_Click(object sender, RoutedEventArgs e)
        {
            string tag = TagTextBox.Text.Trim();

            if (string.IsNullOrEmpty(tag))
            {
                MessageBox.Show("Por favor escribe un tag para filtrar.", "Campo vacío", MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }

            try
            {
                _isLoadingProblems = true;
                DisableButtons();

                ConsoleOutput.Text = $"🔍 Filtrando por tag: '{tag}'...\n";

                var problems = await _apiClient.GetProblemsByTagAsync(tag);

                if (problems.Count == 0)
                {
                    ConsoleOutput.Text += $" No hay problemas con el tag '{tag}'.\n";
                    ProblemList.ItemsSource = null;
                    return;
                }

                ProblemList.ItemsSource = problems;
                ProblemList.DisplayMemberPath = "title";

                ConsoleOutput.Text += $" {problems.Count} problema(s) encontrado(s).\n";
            }
            catch (Exception ex)
            {
                ConsoleOutput.Text += $" ERROR al filtrar: {ex.Message}\n";
            }
            finally
            {
                _isLoadingProblems = false;
                EnableButtons();
            }
        }

        // ==================== Problema Aleatorio ====================
        private async void RandomProblemButton_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                _isLoadingProblems = true;
                DisableButtons();

                ConsoleOutput.Text = " Obteniendo problema aleatorio...\n";

                var problem = await _apiClient.GetRandomProblemAsync();

                if (problem == null)
                {
                    ConsoleOutput.Text += " No hay problemas disponibles.\n";
                    return;
                }

                _currentProblem = problem;

                DescriptionText.Text = $" {problem.title}\n\n" +
                                      $"️ID: {problem.problem_id}\n" +
                                      $"Dificultad: {problem.difficulty}\n" +
                                      $"Tags: {string.Join(", ", problem.tags)}\n\n" +
                                      $"DESCRIPCIÓN:\n{problem.description}\n\n" +
                                      $"CASOS DE PRUEBA:\n{FormatTestCases(problem.test_cases)}";

                CodeEditor.Text = problem.code_stub ?? "// Escribe tu solución aquí\n";

                ConsoleOutput.Text += $"Problema cargado: {problem.title}\n";
            }
            catch (Exception ex)
            {
                ConsoleOutput.Text += $"ERROR: {ex.Message}\n";
            }
            finally
            {
                _isLoadingProblems = false;
                EnableButtons();
            }
        }

        // ==================== Resetear Filtros ====================
        private async void ResetFiltersButton_Click(object sender, RoutedEventArgs e)
        {
            DifficultyComboBox.SelectedIndex = 0;
            TagTextBox.Text = "";
            await LoadProblemListAsync();
        }

        // ==================== Evento: Problema seleccionado ====================
        private async void ProblemList_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (ProblemList.SelectedItem is not ProblemDto selectedProblem)
                return;

            try
            {
                ConsoleOutput.Text = $"Cargando detalles de '{selectedProblem.title}'...\n";

                var fullProblem = await _apiClient.GetProblemAsync(selectedProblem.problem_id);

                if (fullProblem == null)
                {
                    ConsoleOutput.Text += "ERROR: No se pudo cargar el problema completo.\n";
                    return;
                }

                _currentProblem = fullProblem;

                DescriptionText.Text = $"{fullProblem.title}\n\n" +
                                      $"ID: {fullProblem.problem_id}\n" +
                                      $"Dificultad: {fullProblem.difficulty}\n" +
                                      $"Tags: {string.Join(", ", fullProblem.tags)}\n\n" +
                                      $"DESCRIPCIÓN:\n{fullProblem.description}\n\n" +
                                      $"CASOS DE PRUEBA:\n{FormatTestCases(fullProblem.test_cases)}";

                CodeEditor.Text = fullProblem.code_stub ?? "// Escribe tu solución aquí\n";

                ConsoleOutput.Text += "Problema cargado correctamente.\n";
            }
            catch (Exception ex)
            {
                ConsoleOutput.Text += $"ERROR: {ex.Message}\n";
            }
        }

        // ==================== Formatear casos de prueba ====================
        private string FormatTestCases(System.Collections.Generic.List<TestCaseDto> testCases)
        {
            if (testCases == null || testCases.Count == 0)
                return "  (No hay casos de prueba definidos)\n";

            var result = "";
            for (int i = 0; i < testCases.Count; i++)
            {
                result += $"  Test {i + 1}:\n";
                result += $"    Input: {testCases[i].input}\n";
                result += $"    Expected: {testCases[i].expected_output}\n\n";
            }
            return result;
        }

        // ==================== Botón Run ====================
        private async void Run_Click(object sender, RoutedEventArgs e)
        {
            ConsoleOutput.Text = "La función 'Run' aún no está implementada.\n";
            ConsoleOutput.Text += "   → Esta función se conectará al Motor de Evaluación.\n";
            if (_currentProblem == null)
            {
                ConsoleOutput.Text = "❌ No hay un problema seleccionado.\n";
                return;
            }

            string sourceCode = CodeEditor.Text;

            if (string.IsNullOrWhiteSpace(sourceCode))
            {
                MessageBox.Show("El código está vacío.", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
                return;
            }

            ConsoleOutput.Text = "🚀 Enviando solución al motor de evaluación...\n";

            var result = await _apiClient.SubmitSolutionAsync(
                _currentProblem.problem_id,
                "cpp",
                sourceCode,
                2000
            );

            if (result == null)
            {
                ConsoleOutput.Text += "❌ Error al evaluar la solución.\n";
                return;
            }

            ConsoleOutput.Text += $"📌 Resultado global: {result.OverallStatus}\n";
            ConsoleOutput.Text += $"⏱️ Tiempo máximo: {result.MaxTimeMs} ms\n";
            ConsoleOutput.Text += $"💾 Memoria máxima: {result.MaxMemoryKb} KB\n";
            ConsoleOutput.Text += $"📄 Log de compilación:\n{result.CompileLog}\n";

            ConsoleOutput.Text += "\n🔍 Resultados por test:\n";

            foreach (var t in result.Tests)
            {
                ConsoleOutput.Text += $"  ➤ Test {t.Id}: {t.Status} ({t.TimeMs} ms, {t.MemoryKb} KB)\n";
                if (!string.IsNullOrWhiteSpace(t.RuntimeLog))
                {
                    ConsoleOutput.Text += $"     Log:\n{t.RuntimeLog}\n";
                }
            }
        }


        // ==================== Botón Submit ====================
        private async void Submit_Click(object sender, RoutedEventArgs e)
        {
            if (_currentProblem == null)
            {
                MessageBox.Show("Por favor selecciona un problema primero", "Aviso",
                    MessageBoxButton.OK, MessageBoxImage.Information);
                return;
            }

            string code = CodeEditor.Text;

            if (string.IsNullOrWhiteSpace(code))
            {
                MessageBox.Show("Por favor escribe código antes de enviar", "Aviso",
                    MessageBoxButton.OK, MessageBoxImage.Information);
                return;
            }

            ConsoleOutput.Clear();
            ResultOutput.Clear();

            // Submit SOLO analiza la complejidad (NO compila)
            if (_serverConnected)
            {
                ConsoleOutput.Text = "🔍 Analyzing algorithm complexity with AI suggestions...\n";
                ConsoleOutput.AppendText("This may take a few seconds...\n\n");

                // Enviar también el output de la consola
                string consoleText = ConsoleOutput.Text;
                var result = await _analyzerService.AnalyzeCodeAsync(code, _currentProblem.description, consoleText);

                if (result.Success)
                {
                    DisplayAnalysisResults(result);
                }
                else
                {
                    ConsoleOutput.AppendText($"\n⚠ Complexity analysis failed: {result.Error}\n");
                }
            }
        }


        // ==================== Botón Admin ====================
        private void AdminButton_Click(object sender, RoutedEventArgs e)
        {
            var adminWindow = new AdminWindow();
            adminWindow.Show();
        }

        // ==================== Helpers: Deshabilitar/Habilitar botones ====================
        private void DisableButtons()
        {
            if (FilterByTagButton != null) FilterByTagButton.IsEnabled = false;
            if (RandomProblemButton != null) RandomProblemButton.IsEnabled = false;
            if (ResetFiltersButton != null) ResetFiltersButton.IsEnabled = false;
            if (DifficultyComboBox != null) DifficultyComboBox.IsEnabled = false;
        }

        private void EnableButtons()
        {
            if (FilterByTagButton != null) FilterByTagButton.IsEnabled = true;
            if (RandomProblemButton != null) RandomProblemButton.IsEnabled = true;
            if (ResetFiltersButton != null) ResetFiltersButton.IsEnabled = true;
            if (DifficultyComboBox != null) DifficultyComboBox.IsEnabled = true;
        }

        // ==================== Verificar conexión con el servidor de análisis ====================
        private async void CheckServerConnection()
            {
                ConsoleOutput.Text = "Verificando conexión con el Analizodr de Soluciones...\n";
                _serverConnected = await _analyzerService.CheckServerHealthAsync();

                if (_serverConnected)
                {
                    ConsoleOutput.AppendText("Conectado al Analizador\n");
                    ConsoleOutput.AppendText("Listo para analizar la complejidad del algoritmo\n");
                }
                else
                {
                    ConsoleOutput.AppendText("Advertencia: servidor del Analizador no conectado.\n");
                    ConsoleOutput.AppendText("Inicie el servidor C++ en el puerto 8081.\n");
                }
            }

        // ==================== Mostrar resultados del análisis ====================
        private void DisplayAnalysisResults(AnalysisResult result)
        {
            ConsoleOutput.AppendText("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
            ConsoleOutput.AppendText("         RESULTADO DEL ANALISIS DE COMPLEJIDAD\n");
            ConsoleOutput.AppendText("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n");

            
            ConsoleOutput.AppendText($"Tipo de algoritmo: {result.AlgorithmType}\n\n");

            ConsoleOutput.AppendText("Detalles:\n");
            ConsoleOutput.AppendText($"   • Ciclos anidados: {result.Details.NestedLoops}\n");

            ConsoleOutput.AppendText($"   • Average Ratio: {result.Details.AverageRatio:F2}\n\n");

            ConsoleOutput.AppendText($"Explicación:\n   {result.Explanation}\n\n");

            if (result.Suggestions != null && result.Suggestions.Count > 0)
            {
                ConsoleOutput.AppendText("Sugerencias:\n");
                foreach (var suggestion in result.Suggestions)
                {
                    ConsoleOutput.AppendText($"   • {suggestion}\n");
                }
            }

            ConsoleOutput.AppendText("\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
        }

        // ==================== Obtener emoji según la complejidad ====================

        


    }
}