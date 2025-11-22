using System;
using System.Linq;
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

        public MainWindow()
        {
            InitializeComponent();
            _apiClient = new ProblemApiClient("http://localhost:8080");
            Loaded += MainWindow_Loaded;
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

                ConsoleOutput.Text = "🔄 Conectando al Gestor de Problemas...\n";

                bool isHealthy = await _apiClient.IsHealthyAsync();
                if (!isHealthy)
                {
                    ConsoleOutput.Text += "❌ ERROR: No se puede conectar al servidor en http://localhost:8080\n";
                    ConsoleOutput.Text += "   → Asegúrate de que 'problem_manager_api.exe' esté corriendo.\n";
                    ConsoleOutput.Text += "   → Verifica que MongoDB esté activo.\n";
                    return;
                }

                ConsoleOutput.Text += "✅ Conexión exitosa al servidor.\n";

                var problems = await _apiClient.GetAllProblemsAsync();

                if (problems.Count == 0)
                {
                    ConsoleOutput.Text += "\n📭 No hay problemas en la base de datos.\n";
                    ConsoleOutput.Text += "   → Puedes crear algunos usando el Modo Administrador.\n";
                    ProblemList.ItemsSource = null;
                    return;
                }

                ProblemList.ItemsSource = problems;
                ProblemList.DisplayMemberPath = "title";

                ConsoleOutput.Text += $"✅ {problems.Count} problema(s) cargado(s) correctamente.\n\n";
                ConsoleOutput.Text += "💡 Selecciona un problema de la lista para ver sus detalles.\n";
            }
            catch (Exception ex)
            {
                ConsoleOutput.Text += $"❌ ERROR: {ex.Message}\n";
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

                ConsoleOutput.Text = $"🔍 Filtrando por dificultad: {selectedDifficulty}...\n";

                var problems = await _apiClient.GetProblemsByDifficultyAsync(selectedDifficulty);

                if (problems.Count == 0)
                {
                    ConsoleOutput.Text += $"📭 No hay problemas con dificultad '{selectedDifficulty}'.\n";
                    ProblemList.ItemsSource = null;
                    return;
                }

                ProblemList.ItemsSource = problems;
                ProblemList.DisplayMemberPath = "title";

                ConsoleOutput.Text += $"✅ {problems.Count} problema(s) encontrado(s).\n";
            }
            catch (Exception ex)
            {
                ConsoleOutput.Text += $"❌ ERROR al filtrar: {ex.Message}\n";
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
                    ConsoleOutput.Text += $"📭 No hay problemas con el tag '{tag}'.\n";
                    ProblemList.ItemsSource = null;
                    return;
                }

                ProblemList.ItemsSource = problems;
                ProblemList.DisplayMemberPath = "title";

                ConsoleOutput.Text += $"✅ {problems.Count} problema(s) encontrado(s).\n";
            }
            catch (Exception ex)
            {
                ConsoleOutput.Text += $"❌ ERROR al filtrar: {ex.Message}\n";
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

                ConsoleOutput.Text = "🎲 Obteniendo problema aleatorio...\n";

                var problem = await _apiClient.GetRandomProblemAsync();

                if (problem == null)
                {
                    ConsoleOutput.Text += "📭 No hay problemas disponibles.\n";
                    return;
                }

                _currentProblem = problem;

                DescriptionText.Text = $"📝 {problem.title}\n\n" +
                                      $"🏷️  ID: {problem.problem_id}\n" +
                                      $"⚡ Dificultad: {problem.difficulty}\n" +
                                      $"🏷️  Tags: {string.Join(", ", problem.tags)}\n\n" +
                                      $"📄 DESCRIPCIÓN:\n{problem.description}\n\n" +
                                      $"📌 CASOS DE PRUEBA:\n{FormatTestCases(problem.test_cases)}";

                CodeEditor.Text = problem.code_stub ?? "// Escribe tu solución aquí\n";

                ConsoleOutput.Text += $"✅ Problema cargado: {problem.title}\n";
            }
            catch (Exception ex)
            {
                ConsoleOutput.Text += $"❌ ERROR: {ex.Message}\n";
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
                ConsoleOutput.Text = $"📥 Cargando detalles de '{selectedProblem.title}'...\n";

                var fullProblem = await _apiClient.GetProblemAsync(selectedProblem.problem_id);

                if (fullProblem == null)
                {
                    ConsoleOutput.Text += "❌ ERROR: No se pudo cargar el problema completo.\n";
                    return;
                }

                _currentProblem = fullProblem;

                DescriptionText.Text = $"📝 {fullProblem.title}\n\n" +
                                      $"🏷️  ID: {fullProblem.problem_id}\n" +
                                      $"⚡ Dificultad: {fullProblem.difficulty}\n" +
                                      $"🏷️  Tags: {string.Join(", ", fullProblem.tags)}\n\n" +
                                      $"📄 DESCRIPCIÓN:\n{fullProblem.description}\n\n" +
                                      $"📌 CASOS DE PRUEBA:\n{FormatTestCases(fullProblem.test_cases)}";

                CodeEditor.Text = fullProblem.code_stub ?? "// Escribe tu solución aquí\n";

                ConsoleOutput.Text += "✅ Problema cargado correctamente.\n";
            }
            catch (Exception ex)
            {
                ConsoleOutput.Text += $"❌ ERROR: {ex.Message}\n";
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
        private void Run_Click(object sender, RoutedEventArgs e)
        {
            ConsoleOutput.Text = "⚙️ La función 'Run' aún no está implementada.\n";
            ConsoleOutput.Text += "   → Esta función se conectará al Motor de Evaluación.\n";
        }

        // ==================== Botón Submit ====================
        private void Submit_Click(object sender, RoutedEventArgs e)
        {
            ConsoleOutput.Text = "⚙️ La función 'Submit' aún no está implementada.\n";
            ConsoleOutput.Text += "   → Esta función enviará la solución al Motor de Evaluación.\n";
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
    }
}