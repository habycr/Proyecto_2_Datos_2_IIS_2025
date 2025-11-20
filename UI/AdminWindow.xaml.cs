// Archivo: AdminWindow.xaml.cs

using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows;
using System.Windows.Controls;
using UI.Models;
using UI.Services;

namespace UI
{
    public partial class AdminWindow : Window
    {
        private ProblemApiClient _apiClient;
        private ProblemDto? _selectedProblem; // El problema que está siendo editado
        private List<TestCaseDto> _currentTestCases; // Test cases temporales (antes de guardar)

        public AdminWindow()
        {
            InitializeComponent();

            // Inicializar cliente API
            _apiClient = new ProblemApiClient("http://localhost:8080");
            _currentTestCases = new List<TestCaseDto>();

            // Cargar lista al iniciar
            Loaded += AdminWindow_Loaded;
        }

        // ==================== Inicialización ====================
        private async void AdminWindow_Loaded(object sender, RoutedEventArgs e)
        {
            await LoadProblemsAsync();
            ClearForm(); // Formulario vacío al inicio
        }

        // ==================== Cargar lista de problemas ====================
        private async System.Threading.Tasks.Task LoadProblemsAsync()
        {
            try
            {
                Log("🔄 Cargando lista de problemas...");

                bool isHealthy = await _apiClient.IsHealthyAsync();
                if (!isHealthy)
                {
                    Log("❌ ERROR: No se puede conectar al Gestor (http://localhost:8080)");
                    Log("   Asegúrate de que problem_manager_api.exe esté corriendo.");
                    return;
                }

                var problems = await _apiClient.GetAllProblemsAsync();

                ProblemListBox.ItemsSource = problems;

                Log($"✅ {problems.Count} problema(s) cargado(s).");
            }
            catch (Exception ex)
            {
                Log($"❌ ERROR al cargar problemas: {ex.Message}");
            }
        }

        // ==================== Problema seleccionado de la lista ====================
        private async void ProblemListBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (ProblemListBox.SelectedItem is not ProblemDto selectedProblem)
                return;

            try
            {
                Log($"📥 Cargando detalles de '{selectedProblem.title}'...");

                // Obtener el problema completo
                var fullProblem = await _apiClient.GetProblemAsync(selectedProblem.problem_id);

                if (fullProblem == null)
                {
                    Log("❌ ERROR: No se pudo cargar el problema completo.");
                    return;
                }

                _selectedProblem = fullProblem;
                LoadProblemToForm(fullProblem);

                Log($"✅ Problema cargado. Puedes editarlo y presionar 'Guardar'.");
            }
            catch (Exception ex)
            {
                Log($"❌ ERROR: {ex.Message}");
            }
        }

        // ==================== Mapear ProblemDto → Formulario ====================
        private void LoadProblemToForm(ProblemDto problem)
        {
            FormTitle.Text = $"📝 Editando: {problem.title}";

            ProblemIdTextBox.Text = problem.problem_id;
            ProblemIdTextBox.IsEnabled = false; // No se puede cambiar el ID en edición

            TitleTextBox.Text = problem.title;
            DescriptionTextBox.Text = problem.description;
            CodeStubTextBox.Text = problem.code_stub;

            // Dificultad
            DifficultyComboBox.SelectedIndex = problem.difficulty switch
            {
                "Fácil" => 0,
                "Medio" => 1,
                "Difícil" => 2,
                _ => 0
            };

            // Tags
            TagsTextBox.Text = string.Join(", ", problem.tags);

            // Test cases
            _currentTestCases = new List<TestCaseDto>(problem.test_cases);
            RefreshTestCasesList();
        }

        // ==================== Mapear Formulario → ProblemDto ====================
        private ProblemDto? FormToProblemDto()
        {
            try
            {
                // Validaciones básicas
                if (string.IsNullOrWhiteSpace(ProblemIdTextBox.Text))
                {
                    Log("❌ ERROR: El Problem ID no puede estar vacío.");
                    return null;
                }

                if (string.IsNullOrWhiteSpace(TitleTextBox.Text))
                {
                    Log("❌ ERROR: El título no puede estar vacío.");
                    return null;
                }

                if (DifficultyComboBox.SelectedItem == null)
                {
                    Log("❌ ERROR: Debes seleccionar una dificultad.");
                    return null;
                }

                if (_currentTestCases.Count == 0)
                {
                    Log("⚠️ ADVERTENCIA: No hay casos de prueba. El problema se creará sin tests.");
                }

                // Crear el DTO
                var problem = new ProblemDto
                {
                    problem_id = ProblemIdTextBox.Text.Trim(),
                    title = TitleTextBox.Text.Trim(),
                    description = DescriptionTextBox.Text.Trim(),
                    difficulty = (DifficultyComboBox.SelectedItem as ComboBoxItem)?.Content.ToString() ?? "Fácil",
                    code_stub = CodeStubTextBox.Text.Trim(),
                    tags = TagsTextBox.Text
                        .Split(new[] { ',' }, StringSplitOptions.RemoveEmptyEntries)
                        .Select(t => t.Trim())
                        .ToList(),
                    test_cases = new List<TestCaseDto>(_currentTestCases)
                };

                return problem;
            }
            catch (Exception ex)
            {
                Log($"❌ ERROR al procesar el formulario: {ex.Message}");
                return null;
            }
        }

        // ==================== Botón: Nuevo Problema ====================
        private void NewButton_Click(object sender, RoutedEventArgs e)
        {
            ClearForm();
            Log("📝 Formulario limpio. Completa los campos para crear un nuevo problema.");
        }

        // ==================== Botón: Guardar (POST o PUT) ====================
        private async void SaveButton_Click(object sender, RoutedEventArgs e)
        {
            var problem = FormToProblemDto();
            if (problem == null)
                return; // Ya se mostró el error en FormToProblemDto

            try
            {
                bool success;

                // ¿Es un problema nuevo o estamos editando?
                if (_selectedProblem == null ||
                    _selectedProblem.problem_id != problem.problem_id)
                {
                    // POST: Crear nuevo problema
                    Log($"💾 Creando problema '{problem.problem_id}'...");
                    success = await _apiClient.CreateProblemAsync(problem);

                    if (success)
                    {
                        Log($"✅ Problema '{problem.problem_id}' creado exitosamente.");
                        await LoadProblemsAsync();
                        ClearForm();
                    }
                    else
                    {
                        Log($"❌ ERROR: No se pudo crear el problema.");
                        Log("   Verifica que el problem_id no exista ya en la base de datos.");
                    }
                }
                else
                {
                    // PUT: Actualizar problema existente
                    Log($"💾 Actualizando problema '{problem.problem_id}'...");
                    success = await _apiClient.UpdateProblemAsync(problem.problem_id, problem);

                    if (success)
                    {
                        Log($"✅ Problema '{problem.problem_id}' actualizado exitosamente.");
                        await LoadProblemsAsync();
                    }
                    else
                    {
                        Log($"❌ ERROR: No se pudo actualizar el problema.");
                    }
                }
            }
            catch (Exception ex)
            {
                Log($"❌ ERROR al guardar: {ex.Message}");
            }
        }

        // ==================== Botón: Eliminar ====================
        private async void DeleteButton_Click(object sender, RoutedEventArgs e)
        {
            if (ProblemListBox.SelectedItem is not ProblemDto selectedProblem)
            {
                Log("❌ ERROR: Selecciona un problema de la lista primero.");
                return;
            }

            // Confirmación
            var result = MessageBox.Show(
                $"¿Estás seguro de eliminar el problema '{selectedProblem.title}'?\n\n" +
                $"Esta acción NO se puede deshacer.",
                "Confirmar Eliminación",
                MessageBoxButton.YesNo,
                MessageBoxImage.Warning
            );

            if (result != MessageBoxResult.Yes)
                return;

            try
            {
                Log($"🗑️ Eliminando '{selectedProblem.problem_id}'...");

                bool success = await _apiClient.DeleteProblemAsync(selectedProblem.problem_id);

                if (success)
                {
                    Log($"✅ Problema '{selectedProblem.problem_id}' eliminado exitosamente.");
                    await LoadProblemsAsync();
                    ClearForm();
                }
                else
                {
                    Log($"❌ ERROR: No se pudo eliminar el problema.");
                }
            }
            catch (Exception ex)
            {
                Log($"❌ ERROR al eliminar: {ex.Message}");
            }
        }

        // ==================== Botón: Recargar Lista ====================
        private async void RefreshButton_Click(object sender, RoutedEventArgs e)
        {
            await LoadProblemsAsync();
        }

        // ==================== Test Cases: Agregar ====================
        private void AddTestCaseButton_Click(object sender, RoutedEventArgs e)
        {
            string input = TestInputTextBox.Text.Trim();
            string output = TestOutputTextBox.Text.Trim();

            if (string.IsNullOrWhiteSpace(input) || string.IsNullOrWhiteSpace(output))
            {
                Log("⚠️ ADVERTENCIA: Input y Output no pueden estar vacíos.");
                return;
            }

            _currentTestCases.Add(new TestCaseDto
            {
                input = input,
                expected_output = output
            });

            RefreshTestCasesList();

            // Limpiar los TextBox
            TestInputTextBox.Clear();
            TestOutputTextBox.Clear();

            Log($"✅ Caso de prueba agregado: '{input}' → '{output}'");
        }

        // ==================== Test Cases: Eliminar ====================
        private void RemoveTestCaseButton_Click(object sender, RoutedEventArgs e)
        {
            if (TestCasesListBox.SelectedItem is not string selectedItem)
            {
                Log("⚠️ ADVERTENCIA: Selecciona un caso de prueba primero.");
                return;
            }

            int index = TestCasesListBox.SelectedIndex;
            if (index >= 0 && index < _currentTestCases.Count)
            {
                _currentTestCases.RemoveAt(index);
                RefreshTestCasesList();
                Log($"✅ Caso de prueba eliminado.");
            }
        }

        // ==================== Refrescar lista visual de test cases ====================
        private void RefreshTestCasesList()
        {
            var displayList = _currentTestCases
                .Select(tc => $"{tc.input} → {tc.expected_output}")
                .ToList();

            TestCasesListBox.ItemsSource = displayList;
        }

        // ==================== Limpiar formulario ====================
        private void ClearForm()
        {
            _selectedProblem = null;
            FormTitle.Text = "📝 Nuevo Problema";

            ProblemIdTextBox.Clear();
            ProblemIdTextBox.IsEnabled = true; // Habilitado para nuevos problemas

            TitleTextBox.Clear();
            DescriptionTextBox.Clear();
            CodeStubTextBox.Clear();
            TagsTextBox.Clear();

            DifficultyComboBox.SelectedIndex = 0; // Fácil por defecto

            _currentTestCases.Clear();
            RefreshTestCasesList();

            TestInputTextBox.Clear();
            TestOutputTextBox.Clear();

            ProblemListBox.SelectedItem = null;
        }

        // ==================== Helper: Log a consola ====================
        private void Log(string message)
        {
            ConsoleTextBox.AppendText($"{DateTime.Now:HH:mm:ss} | {message}\n");
            ConsoleTextBox.ScrollToEnd();
        }
    }
}