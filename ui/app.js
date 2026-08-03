let state = {
  pendingCount: 0,
  availableCount: 0,
  waitingCount: 0,
  pending: [],
  resources: [],
  orderedResources: [],
  patients: [],
  waiting: [],
  history: [],
};

const ids = {
  pendingCount: 'pendingCount',
  availableCount: 'availableCount',
  waitingCount: 'waitingCount',
  heapList: 'heapList',
  resourceList: 'resourceList',
  orderedResourceList: 'orderedResourceList',
  patientList: 'patientList',
  waitingList: 'waitingList',
  historyList: 'historyList',
  routePreview: 'routePreview',
};

const els = Object.fromEntries(Object.entries(ids).map(([key, value]) => [key, document.getElementById(value)]));

function renderList(container, items, itemRenderer, emptyText) {
  container.innerHTML = '';
  if (!items || items.length === 0) {
    container.innerHTML = `<div class="item"><strong>${emptyText}</strong></div>`;
    return;
  }

  items.forEach((item, index) => {
    const element = document.createElement('div');
    element.className = 'item';
    if (index === 0 && container === els.heapList) {
      element.classList.add('top');
    }
    if (container === els.waitingList) {
      element.classList.add('waiting');
    }
    element.innerHTML = itemRenderer(item, index);
    container.appendChild(element);
  });
}

function renderRoutePreview(route) {
  const container = els.routePreview;
  if (!route) {
    container.classList.add('empty');
    container.textContent = 'No route loaded yet.';
    return;
  }

  container.classList.remove('empty');
  container.innerHTML = `
    <strong>Patient #${route.patientId} to Resource #${route.resourceId}</strong>
    <div class="meta">${escapeHtml(route.path.join(' -> '))} | distance ${route.distance} | from ${route.fromLocation} to ${route.toLocation}</div>
  `;
}

function render() {
  document.getElementById(ids.pendingCount).textContent = state.pendingCount ?? 0;
  document.getElementById(ids.availableCount).textContent = state.availableCount ?? 0;
  document.getElementById(ids.waitingCount).textContent = state.waitingCount ?? 0;

  renderList(
    els.heapList,
    state.pending,
    (emergency, index) => `
      <strong>${index === 0 ? 'Top priority: ' : ''}${escapeHtml(`#${emergency.id} ${emergency.patientName}`)}</strong>
      <div class="meta">Severity ${emergency.severity} | ${escapeHtml(emergency.type)} | needs ${escapeHtml(emergency.requiredResourceType)} | t=${emergency.arrivalTime}</div>
    `,
    'No pending emergencies'
  );

  renderList(
    els.resourceList,
    state.resources,
    (resource) => `
      <strong>#${resource.id} ${escapeHtml(resource.type)}</strong>
      <div class="meta">${resource.available ? 'Available' : `Busy for #${resource.assignedEmergencyId} ${escapeHtml(resource.assignedEmergencyName)}`} | location ${resource.location}</div>
    `,
    'No resources added'
  );

  renderList(
    els.orderedResourceList,
    state.orderedResources,
    (resource) => `
      <strong>#${resource.id} ${escapeHtml(resource.type)}</strong>
      <div class="meta">BST order | ${resource.available ? 'Available' : `Busy for #${resource.assignedEmergencyId} ${escapeHtml(resource.assignedEmergencyName)}`} | location ${resource.location}</div>
    `,
    'No ordered resources'
  );

  renderList(
    els.patientList,
    state.patients,
    (patient) => `
      <strong>#${patient.patientId} ${escapeHtml(patient.patientName)}</strong>
      <div class="meta">${escapeHtml(patient.conditionType)} | severity ${patient.severity} | needs ${escapeHtml(patient.requiredResourceType)} | loc ${patient.location} | ${escapeHtml(patient.status)}</div>
    `,
    'No patient records'
  );

  renderList(
    els.waitingList,
    state.waiting,
    (emergency) => `
      <strong>#${emergency.id} ${escapeHtml(emergency.patientName)}</strong>
      <div class="meta">Severity ${emergency.severity} | waiting for ${escapeHtml(emergency.requiredResourceType)}</div>
    `,
    'No waiting cases'
  );

  renderList(
    els.historyList,
    state.history,
    (record) => `
      <strong>Emergency ${record.emergencyId} → Resource ${record.resourceId}</strong>
      <div class="meta">${escapeHtml(record.status)} | t=${record.timeStamp}</div>
    `,
    'No history yet'
  );
}

async function refreshState() {
  const response = await fetch('/api/state');
  if (!response.ok) {
    throw new Error('Could not fetch allocator state.');
  }
  state = await response.json();
  render();
}

async function postAction(path, entries = {}) {
  const response = await fetch(path, {
    method: 'POST',
    headers: {
      'Content-Type': 'application/x-www-form-urlencoded;charset=UTF-8',
    },
    body: new URLSearchParams(entries),
  });

  const payload = await response.text();
  let parsed = null;
  try {
    parsed = payload ? JSON.parse(payload) : null;
  } catch {
    parsed = null;
  }

  if (!response.ok) {
    throw new Error(parsed?.error || 'Request failed.');
  }

  await refreshState();
}

function escapeHtml(value) {
  return String(value)
    .replaceAll('&', '&amp;')
    .replaceAll('<', '&lt;')
    .replaceAll('>', '&gt;')
    .replaceAll('"', '&quot;')
    .replaceAll("'", '&#039;');
}

function wireForms() {
  document.getElementById('resourceForm').addEventListener('submit', async event => {
    event.preventDefault();
    try {
      await postAction('/api/resource', {
        id: document.getElementById('resourceId').value,
        type: document.getElementById('resourceType').value.trim(),
        location: document.getElementById('resourceLocation').value,
      });
      event.target.reset();
    } catch (error) {
      alert(error.message);
    }
  });

  document.getElementById('emergencyForm').addEventListener('submit', async event => {
    event.preventDefault();
    try {
      await postAction('/api/emergency', {
        id: document.getElementById('emergencyId').value,
        patientName: document.getElementById('patientName').value.trim(),
        severity: document.getElementById('severity').value,
        type: document.getElementById('emergencyType').value.trim(),
        requiredResourceType: document.getElementById('requiredResourceType').value.trim(),
        location: document.getElementById('patientLocation').value,
      });
      event.target.reset();
    } catch (error) {
      alert(error.message);
    }
  });

  document.getElementById('roadForm').addEventListener('submit', async event => {
    event.preventDefault();
    try {
      await postAction('/api/road', {
        from: document.getElementById('roadFrom').value,
        to: document.getElementById('roadTo').value,
        weight: document.getElementById('roadWeight').value,
      });
      event.target.reset();
    } catch (error) {
      alert(error.message);
    }
  });

  document.getElementById('routeForm').addEventListener('submit', async event => {
    event.preventDefault();
    try {
      const patientId = document.getElementById('routePatientId').value;
      const resourceId = document.getElementById('routeResourceId').value.trim();
      const query = new URLSearchParams({ patientId });
      if (resourceId) {
        query.set('resourceId', resourceId);
      }
      const response = await fetch(`/api/route?${query.toString()}`);
      const payload = await response.text();
      let parsed = null;
      try {
        parsed = payload ? JSON.parse(payload) : null;
      } catch {
        parsed = null;
      }
      if (!response.ok) {
        throw new Error(parsed?.error || 'Request failed.');
      }
      renderRoutePreview(parsed);
    } catch (error) {
      alert(error.message);
    }
  });

  document.getElementById('serveNextBtn').addEventListener('click', async () => {
    try {
      await postAction('/api/serve-next');
    } catch (error) {
      alert(error.message);
    }
  });

  document.getElementById('updateDemoBtn').addEventListener('click', async () => {
    try {
      await postAction('/api/update-severity', { id: '3', severity: '10' });
    } catch (error) {
      alert(error.message);
    }
  });

  document.getElementById('releaseDemoBtn').addEventListener('click', async () => {
    try {
      await postAction('/api/release-resource', { resourceId: '201' });
    } catch (error) {
      alert(error.message);
    }
  });

  document.getElementById('demoBtn').addEventListener('click', async () => {
    try {
      await postAction('/api/demo');
    } catch (error) {
      alert(error.message);
    }
  });

  document.getElementById('resetBtn').addEventListener('click', async () => {
    try {
      await postAction('/api/reset');
      renderRoutePreview(null);
    } catch (error) {
      alert(error.message);
    }
  });
}

wireForms();
refreshState().catch(error => alert(error.message));
